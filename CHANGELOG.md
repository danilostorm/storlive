# Changelog

## 0.1.2

- Corrige `OBS_VIDEO_MODULE_NOT_FOUND (-5)` ao resolver o módulo gráfico do libobs pelo runtime real do sistema.
- Linux agora usa o módulo OpenGL versionado disponível no pacote `libobs0t64`, sem depender do symlink instalado por `libobs-dev`.
- Windows resolve `libobs-d3d11.dll` a partir da própria pasta do portable.
- CI instala e abre o `.deb` depois de remover `libobs-dev`, reproduzindo uma instalação de usuário final.
- CI extrai o ZIP Windows em outra pasta e inicia o aplicativo com diretório de trabalho independente.
- CI passa a exigir confirmação explícita de que o engine libobs inicializou, e não apenas que a janela permaneceu aberta.
- A versão exibida na janela passa a acompanhar automaticamente a versão do build.

## 0.1.1

- Corrige dependências Qt/QML ausentes nos pacotes de runtime.
- Inclui o runtime MSVC necessário no portable Windows.
- Adiciona smoke tests de abertura da interface em Linux e Windows.
- Evita consultar fontes do libobs quando a inicialização do engine falha.

## 0.1.0

- Desktop rewrite in C++20 + Qt Quick.
- Direct libobs scene/capture/audio integration.
- Real preview and source property editor.
- Source visibility, mute and removal controls.
- Default visual composition for capture and webcam.
- 720p/1080p, 30/60 fps and bitrate configuration.
- H.264 hardware/automatic/x264 encoder selection and AAC audio.
- Multi-output RTMP with shared encode groups, independent services, reconnection and statistics.
- Debian/Ubuntu `.deb` packaging.
- Windows x64 portable packaging with embedded libobs/OBS 30.0.2 runtime.
- CI safeguards preventing a Windows UI-only stub from being published.
- Tagged GitHub Releases with SHA-256 checksums.
