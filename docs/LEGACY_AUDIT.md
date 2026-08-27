# Auditoria do sistema legado `hoststorm-lofi-suite/multi-live`

A aplicação antiga foi analisada apenas para identificar conceitos reaproveitáveis. Nenhuma chave de transmissão foi copiada para este repositório.

## O que existe no legado

O `multi-live/app.py` é um monólito Python/Flask de aproximadamente 1.400 linhas. Ele concentra:

- configuração de canais em `channels.json`;
- YouTube, Twitch, Kick, Custom RTMP, YouTube Shorts e Kwai;
- execução de múltiplos processos FFmpeg;
- watchdog 24/7;
- upload/remoção de mídia;
- fontes locais e por URL/yt-dlp;
- modos horizontal/vertical;
- browser overlay/Chromium/Xvfb;
- estado por PID;
- interface HTML;
- muitos backups `*.bak_*` dentro da própria árvore de produção.

## Conceito que vale preservar

O legado usa o muxer `tee` do FFmpeg quando vários destinos normais compartilham o mesmo encode, com `onfail=ignore`. Esse é o comportamento conceitual correto: **codificar uma vez e distribuir para vários destinos compatíveis**.

No StorLive esse conceito deixa de depender do comando FFmpeg monolítico e passa a ser representado explicitamente pelo `MultiOutputManager`, agrupando destinos por perfil de encode.

## O que NÃO será migrado

- Flask/web UI;
- Docker como requisito do aplicativo;
- watchdog de processo baseado em `/proc/PID`;
- uploads e biblioteca de mídia;
- yt-dlp;
- loop 24/7 de arquivos;
- Shorts/Kwai como pipelines especiais;
- Chromium/Xvfb overlay legado;
- backups `app.py.bak_*`, `index.html.bak_*` e `Dockerfile.bak_*`;
- estado persistido com PID;
- credenciais do `channels.json`.

## O que pode inspirar a nova implementação

- lista de destinos e RTMP customizado;
- estados independentes por plataforma;
- reinício/reconexão por output;
- máscara de stream key em logs;
- agrupamento de destinos compatíveis;
- possibilidade futura de perfil vertical separado, sem acoplar isso ao core.
