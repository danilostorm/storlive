#include "ObsPreviewProvider.h"

#include <QMetaObject>
#include <QMutexLocker>

ObsPreviewProvider::ObsPreviewProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

ObsPreviewProvider::~ObsPreviewProvider()
{
    stop();
}

bool ObsPreviewProvider::start()
{
#ifdef STORLIVE_HAS_LIBOBS
    if (active())
        return true;
    if (!obs_initialized())
        return false;

    m_video = obs_get_video();
    if (!m_video)
        return false;

    const video_output_info *info = video_output_get_info(m_video);
    if (!info || info->width == 0 || info->height == 0) {
        m_video = nullptr;
        return false;
    }

    m_width = info->width > 960 ? 960u : info->width;
    m_height = static_cast<uint32_t>((static_cast<uint64_t>(m_width) * info->height) / info->width);
    if (m_height < 2)
        m_height = 2;
    if (m_height % 2)
        --m_height;

    video_scale_info conversion {};
    conversion.format = VIDEO_FORMAT_BGRA;
    conversion.width = m_width;
    conversion.height = m_height;
    conversion.range = VIDEO_RANGE_FULL;
    conversion.colorspace = VIDEO_CS_709;

    const double fps = video_output_get_frame_rate(m_video);
    const uint32_t divisor = fps > 35.0 ? 2u : 1u;
    const bool connected = video_output_connect2(m_video, &conversion, divisor, &ObsPreviewProvider::onVideoFrame, this);
    m_active.store(connected, std::memory_order_relaxed);
    if (!connected)
        m_video = nullptr;
    emit activeChanged();
    return connected;
#else
    return false;
#endif
}

void ObsPreviewProvider::stop()
{
#ifdef STORLIVE_HAS_LIBOBS
    if (m_video && active())
        video_output_disconnect(m_video, &ObsPreviewProvider::onVideoFrame, this);
    m_video = nullptr;
#endif

    const bool wasActive = m_active.exchange(false, std::memory_order_relaxed);
    if (wasActive)
        emit activeChanged();
}

QImage ObsPreviewProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)

    QImage image;
    {
        QMutexLocker locker(&m_mutex);
        image = m_frame;
    }

    if (image.isNull()) {
        image = QImage(static_cast<int>(m_width), static_cast<int>(m_height), QImage::Format_ARGB32);
        image.fill(Qt::black);
    }

    if (size)
        *size = image.size();

    if (requestedSize.isValid() && requestedSize != image.size())
        return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}

#ifdef STORLIVE_HAS_LIBOBS
void ObsPreviewProvider::onVideoFrame(void *param, video_data *frame)
{
    if (!param || !frame)
        return;
    static_cast<ObsPreviewProvider *>(param)->handleFrame(frame);
}

void ObsPreviewProvider::handleFrame(video_data *frame)
{
    if (!frame->data[0] || frame->linesize[0] == 0)
        return;

    const QImage wrapped(frame->data[0],
                         static_cast<int>(m_width),
                         static_cast<int>(m_height),
                         static_cast<qsizetype>(frame->linesize[0]),
                         QImage::Format_ARGB32);
    const QImage copy = wrapped.copy();

    {
        QMutexLocker locker(&m_mutex);
        m_frame = copy;
    }

    bool expected = false;
    if (!m_updatePending.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;

    QMetaObject::invokeMethod(this, [this]() {
        m_revision.fetch_add(1, std::memory_order_relaxed);
        m_updatePending.store(false, std::memory_order_release);
        emit frameChanged();
    }, Qt::QueuedConnection);
}
#endif
