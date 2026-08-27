# StorLive

StorLive é um aplicativo desktop focado em **captura + multi-live**, para Windows e Linux. A proposta é manter somente o que interessa para transmissão: cenas/fontes, preview, áudio, encoder e vários destinos simultâneos.

## Objetivo

- Captura de jogo, janela, monitor e webcam.
- Microfone e áudio do desktop.
- Cenas e fontes em uma interface menor que o OBS Studio.
- Encoder automático, hardware ou software.
- NVIDIA NVENC, AMD AMF, Intel Quick Sync e x264 conforme disponibilidade do libobs/plugins.
- YouTube, Twitch, Kick, Facebook e RTMP customizado.
- Um mesmo perfil de encode compartilhado entre destinos compatíveis.
- Falha/reconexão independente por destino.
- Windows em pacote portable ZIP.
- Linux em pacote `.deb` para Debian/Ubuntu.

## Estado atual — 0.1.0-dev

Esta primeira base já contém:

- projeto C++20 + Qt 6.4+/QML;
- separação `UI -> Controller -> Core -> libobs`;
- inicialização opcional de libobs;
- modelo de cenas/fontes e destinos;
- agrupamento de destinos por perfil de encode para permitir encode compartilhado;
- interface desktop inicial;
- empacotamento CPack `.deb`;
- script para Windows portable;
- GitHub Actions para Windows e Linux;
- auditoria da aplicação legada em `docs/LEGACY_AUDIT.md`.

O pipeline RTMP e as fontes reais de captura estão sendo implementados em etapas separadas para evitar transformar o novo código em outro monólito.

## Build rápido

### Linux / Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=OFF
cmake --build build
./build/storlive
```

Para compilar com libobs instalado:

```bash
sudo apt install -y libobs-dev obs-studio
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON
cmake --build build
```

### Windows

Use Qt 6.4+ e CMake/Ninja. Para o build de interface sem libobs:

```powershell
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=OFF
cmake --build build --config Release
cmake --install build --prefix dist
powershell -ExecutionPolicy Bypass -File packaging/windows/make-portable.ps1 -InstallDir dist -OutputDir artifacts
```

Quando um bundle do OBS/libobs for usado no Windows, informe `STORLIVE_OBS_ROOT` no CMake.

## Organização

```text
src/app              inicialização do aplicativo
src/core/obs         integração libobs
src/core/streaming   perfis, destinos e multi-output
src/core             controller da aplicação
resources/qml        interface Qt Quick
packaging/windows    portable ZIP
packaging/linux      metadados de pacote
docs                 arquitetura e migração
.github/workflows    builds Windows/Linux
```

## Segurança

Chaves de transmissão do sistema antigo **não são importadas nem commitadas**. O novo projeto deverá armazenar segredos no cofre nativo do sistema operacional antes de oferecer persistência de stream keys.

## Licença

GPL-2.0-or-later, compatível com a integração direta com libobs. Veja `LICENSE`.
