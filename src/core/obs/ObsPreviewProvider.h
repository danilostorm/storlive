#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

#include <atomic>
#include <cstdint>

#ifdef STORLIVE_HAS_LIBOBS
extern "C" {
#include <obs.h>
#include <media-io/video-io.h>
}
#endif

class ObsPreviewProvider final : public QQuickImageProvider
{
    Q_OBJECT
    Q_PROPERTY(qulonglong revision READ revision NOTIFY frameChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    ObsPreviewProvider();
    ~ObsPreviewProvider() override;

    bool start();
    void stop();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    qulonglong revision() const { return m_revision.load(std::memory_order_relaxed); }
    bool active() const { return m_active.load(std::memory_order_relaxed); }

signals:
    void frameChanged();
    void activeChanged();

private:
#ifdef STORLIVE_HAS_LIBOBS
    static void onVideoFrame(void *param, video_data *frame);
    void handleFrame(video_data *frame);
    video_t *m_video {nullptr};
#endif

    QMutex m_mutex;
    QImage m_frame;
    uint32_t m_width {960};
    uint32_t m_height {540};
    std::atomic<qulonglong> m_revision {0};
    std::atomic_bool m_active {false};
    std::atomic_bool m_updatePending {false};
};
