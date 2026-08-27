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

A base atual contém:

- projeto C++20 + Qt 6.4+/QML;
- inicialização real de libobs no build Linux;
- áudio 48 kHz e vídeo base 1080p60;
- cena `Gameplay` real do libobs;
- detecção/adicionamento das fontes de captura fornecidas pelos plugins instalados;
- editor genérico das propriedades OBS da fonte (listas, bool, inteiro, decimal e texto), permitindo escolher janela/dispositivo quando o plugin expõe essa opção;
- encoder H.264 automático/hardware/software + AAC;
- multi-output RTMP real no backend libobs;
- compartilhamento do mesmo par de encoders entre destinos com perfil idêntico;
- reconexão e métricas por destino;
- configuração de servidor/stream key pela interface sem exibir a chave de volta;
- empacotamento CPack `.deb`;
- Windows portable ZIP;
- GitHub Actions para Windows e Linux;
- auditoria da aplicação legada em `docs/LEGACY_AUDIT.md`.

### Limites atuais

- O preview ainda é um placeholder no Qt Quick; a cena já alimenta a saída do libobs, mas falta renderizá-la dentro da janela.
- Propriedades OBS especiais como botões, seletor de arquivos avançado, fontes e frame-rate ainda não possuem controles próprios; as propriedades comuns de seleção/configuração já são tratadas.
- O CI Linux compila com libobs. O Windows portable ainda compila a UI/core sem libobs enquanto o SDK/runtime do OBS é integrado ao pacote MSVC.
- Stream keys ficam somente em memória nesta fase; nenhuma chave do sistema legado foi importada.

## Build rápido

### Linux / Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev libobs-dev obs-plugins
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON
cmake --build build
./build/storlive
```

### Windows

Use Qt 6.4+ com toolchain MSVC e CMake/Ninja. O scaffold atual pode ser compilado sem libobs:

```powershell
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=OFF -DCMAKE_CXX_COMPILER=cl
cmake --build build --config Release
cmake --install build --prefix dist
powershell -ExecutionPolicy Bypass -File packaging/windows/make-portable.ps1 -InstallDir dist -OutputDir artifacts
```

Quando um bundle/SDK compatível do OBS/libobs for integrado ao Windows, o CMake já aceita `STORLIVE_OBS_ROOT`.

## Organização

```text
src/app              inicialização do aplicativo
src/core/obs         engine, cenas e fontes libobs
src/core/streaming   perfis, destinos e multi-output
src/core             controller da aplicação
resources/qml        interface Qt Quick
packaging/windows    portable ZIP
packaging/linux      metadados de pacote
docs                 arquitetura e migração
.github/workflows    builds Windows/Linux
```

## Segurança

Chaves de transmissão do sistema antigo **não são importadas nem commitadas**. A persistência futura deverá usar o cofre nativo do sistema operacional.

## Licença

GPL-2.0-or-later, compatível com a integração direta com libobs. Veja `LICENSE`.
