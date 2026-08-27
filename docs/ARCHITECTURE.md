# Arquitetura do StorLive

## Princípio

StorLive não tenta duplicar toda a interface do OBS Studio. O objetivo é usar o libobs como engine e manter uma camada de produto pequena, centrada em multi-live.

```text
Qt/QML UI
   |
AppController
   |
   +-- Scene/Source model
   +-- Audio model
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

Destinos ativos com a mesma assinatura entram no mesmo grupo. O backend deve criar **um encoder de vídeo e um encoder de áudio por grupo** e compartilhar esses encoders entre os `obs_output_t` do grupo.

Se uma plataforma exigir outro bitrate/resolução/codec, ela recebe outro grupo.

## Falhas independentes

Cada destino deverá manter seu próprio estado:

- Parado
- Conectando
- Ao vivo
- Reconectando
- Erro

Uma falha em um `obs_output_t` não deve parar os demais outputs.

## Captura

A API de UI será comum, mas a implementação das fontes será específica por plataforma:

### Windows

- Game Capture
- Window Capture
- Display Capture
- WASAPI input/output
- DirectShow/webcam via plugin OBS

### Linux

- PipeWire/Wayland
- X11 quando disponível
- V4L2 webcam
- PipeWire/PulseAudio
- captura de jogo conforme plugins disponíveis

## Segredos

Stream keys não pertencem a JSON do projeto nem ao Git. Persistência deverá usar Credential Manager no Windows e Secret Service/libsecret no Linux.
