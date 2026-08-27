# Arquitetura do StorLive

## Princípio

StorLive não tenta duplicar toda a interface do OBS Studio. O objetivo é usar o libobs como engine e manter uma camada de produto pequena, centrada em multi-live.

```text
Qt/QML UI
   |
AppController
   |
   +-- SceneManager -> obs_scene_t -> fontes de captura
   +-- Audio/Video 1080p60 / 48 kHz
   +-- Encoder selection
   +-- MultiOutputManager
             |
             +-- Encode profile A -> YouTube / Twitch / Kick
             +-- Encode profile B -> outro bitrate/resolução
   |
ObsEngine / libobs
```

## Regra de encode compartilhado

Cada destino recebe uma assinatura de perfil:

`codec:widthxheight:fps:videoBitrate:audioBitrate`

Destinos ativos com a mesma assinatura entram no mesmo grupo. O backend cria **um encoder de vídeo e um encoder de áudio por grupo** e associa esses mesmos encoders a todos os `obs_output_t` daquele grupo.

Se uma plataforma exigir outro bitrate/resolução/codec, ela recebe outro grupo.

## Outputs

Cada destino possui:

- `rtmp_custom` service próprio;
- `rtmp_output` próprio;
- servidor e stream key próprios;
- reconexão configurada individualmente;
- métricas de bytes, frames, dropped frames, congestionamento e tempo de conexão.

Assim uma falha de rede de um destino não obriga o app a derrubar os demais.

## Encoder

O backend enumera os encoders registrados no libobs e procura H.264. Em `Automático`, prioriza hardware (NVENC/AMF/QSV/VAAPI) e cai para x264. Em `Hardware`, exige um encoder de hardware. Em `Software`, prioriza x264. Para áudio, procura AAC e prioriza `ffmpeg_aac` quando disponível.

## Captura

A UI oferece tipos comuns, enquanto `SceneManager` resolve o plugin disponível no sistema.

### Windows

- `game_capture`
- `window_capture`
- `monitor_capture`
- `dshow_input`
- `wasapi_input_capture`
- `wasapi_output_capture`

### Linux

- PipeWire/Wayland quando os plugins correspondentes existem;
- X11/XComposite como fallback;
- V4L2 webcam;
- PulseAudio/PipeWire para áudio.

A cena `Gameplay` é ligada ao canal principal do libobs com `obs_set_output_source`.

## Preview

A composição já alimenta o pipeline do libobs, mas o preview ainda não é renderizado dentro do Qt Quick. Essa integração gráfica fica isolada para não misturar captura/transmissão com a UI.

## Segredos

Stream keys não pertencem a JSON do projeto nem ao Git. Nesta fase elas ficam somente em memória. Persistência futura deverá usar Credential Manager no Windows e Secret Service/libsecret no Linux.
