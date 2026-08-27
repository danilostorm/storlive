# StorLive

StorLive é um aplicativo desktop focado em **captura + multi-live** para Windows e Linux. A proposta é manter somente o que interessa para transmissão: fontes, preview, áudio, encoder e vários destinos RTMP simultâneos, sem carregar a interface completa do OBS Studio.

## Versão atual: v0.1.1

A v0.1.1 corrige os problemas de inicialização da primeira release. O CI agora não valida somente compilação: ele **abre o aplicativo de verdade** em Windows e Linux e bloqueia a publicação se o processo encerrar durante o startup.

Correções principais:

- caminho do QML embutido corrigido para `qrc:/StorLive/Main.qml`;
- runtime MSVC incluído no ZIP portable do Windows;
- módulos Qt Quick/QML necessários incluídos/verificados no Windows;
- dependências Qt Quick/QML declaradas no `.deb` Linux;
- acesso ao registro de fontes do libobs protegido quando o engine não consegue inicializar, evitando encerramento por access violation;
- log de startup em `storlive-startup.log` para diagnóstico.

## Recursos

- Captura de jogo, janela, monitor/tela e webcam através dos plugins do libobs.
- Microfone e áudio do computador.
- Cena `Gameplay` com preview real da composição.
- Propriedades reais das fontes OBS para selecionar janela, monitor, câmera e dispositivos de áudio.
- Mostrar/ocultar fonte, mutar/desmutar fontes de áudio e remover fontes.
- Encoder H.264 em modo **Automático**, **Hardware** ou **Software (x264)**.
- NVIDIA NVENC, AMD AMF e Intel Quick Sync conforme GPU, driver e plugin disponíveis.
- Perfil configurável: 1920×1080 ou 1280×720, 30/60 fps, bitrate de vídeo e áudio.
- YouTube, Twitch, Kick, Facebook e RTMP personalizado.
- Vários destinos simultâneos usando o mesmo encoder quando compartilham o mesmo perfil.
- Reconexão independente e métricas por destino.
- Windows em ZIP portable, sem instalador e sem exigir OBS Studio instalado.
- Linux em `.deb` para Debian/Ubuntu.

## Windows portable

O CI monta um runtime próprio baseado no **OBS Studio/libobs 30.0.2**, gera a import library MSVC a partir do `obs.dll` oficial e compila o StorLive com `STORLIVE_WITH_LIBOBS=ON`. O ZIP final inclui o runtime necessário para captura, áudio, encoders, RTMP e o runtime Visual C++ requerido pelo executável.

Plugins principais empacotados:

- `obs-outputs` e `rtmp-services` para RTMP;
- `obs-x264` e `obs-ffmpeg` para encode;
- `obs-nvenc` e `obs-qsv11` quando presentes no runtime;
- `win-capture` para jogo/janela/monitor;
- `win-dshow` para webcam;
- `win-wasapi` para microfone e áudio do computador.

Antes de publicar, o CI verifica `obs.dll`, D3D11, plugins de captura/RTMP, runtime MSVC, plataforma Qt, módulos QML e então inicia `storlive.exe`. O processo precisa permanecer aberto durante o smoke test.

## Linux

No Ubuntu 24.04 o build usa `libobs-dev` + `obs-plugins`. O pacote `.deb` declara também os módulos QML usados pela interface (`QtQuick`, Controls, Layouts, Templates, Window e WorkerScript). O CI valida a ligação com `libobs.so`, inicia a interface em ambiente gráfico virtual e só depois gera/aceita o pacote.

### Build local Linux

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
  qt6-base-dev qt6-declarative-dev \
  qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
  qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml-workerscript \
  libobs-dev obs-plugins
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack -G DEB
```

## Build local Windows

Requisitos: Windows x64, Visual Studio 2022/MSVC, CMake/Ninja e Qt 6.8.

```powershell
./packaging/windows/prepare-obs-runtime.ps1 -Version 30.0.2 -WorkDir .obs-runtime
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON -DSTORLIVE_OBS_ROOT="$pwd/.obs-runtime/sdk" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=cl
cmake --build build
cmake --install build --prefix dist
./packaging/windows/make-portable.ps1 -InstallDir dist -OutputDir artifacts -ObsRuntimeRoot .obs-runtime/runtime
```

## Releases

Uma release só é publicada depois dos jobs Windows e Linux concluírem com sucesso, incluindo os testes de startup. Os arquivos publicados são:

- `StorLive-Windows-x64-portable.zip`;
- `storlive_<versão>_amd64.deb`;
- `SHA256SUMS.txt`.

## Organização

```text
src/app              inicialização do aplicativo
src/core/obs         engine, cena, fontes e preview libobs
src/core/streaming   perfis, destinos, encoders e multi-output
src/core             controller da aplicação
resources/qml        interface Qt Quick
packaging/windows    runtime OBS e portable ZIP
packaging/linux      metadados do pacote Debian
docs                 arquitetura e auditoria da base legada
.github/workflows    build, validação, smoke tests e release
```

## Segurança

Nenhuma chave de transmissão do sistema legado é importada ou commitada. As stream keys informadas na interface permanecem apenas em memória nesta versão e não são exibidas de volta pela UI.

## Licença

StorLive é GPL-2.0-or-later, compatível com a integração direta com libobs/OBS Studio. O portable inclui aviso de terceiros e referência ao código-fonte do OBS Project. Veja `LICENSE`.
