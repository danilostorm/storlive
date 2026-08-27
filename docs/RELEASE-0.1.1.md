# StorLive v0.1.1

Esta versão corrige os problemas de inicialização encontrados na v0.1.0.

## Correções de startup

- Corrigido o caminho do QML embutido para `qrc:/StorLive/Main.qml`.
- Windows portable agora inclui o runtime MSVC (`MSVCP140*` e `VCRUNTIME140*`).
- Windows portable inclui e valida os módulos Qt Quick/QML necessários.
- Linux `.deb` declara os módulos QML necessários (`QtQuick`, Controls, Layouts, Templates, Window e WorkerScript).
- O frontend não consulta o registro de fontes do libobs quando a inicialização do engine falha; em vez de encerrar com access violation, a janela abre e mostra o erro do engine.
- Adicionado log de inicialização em `storlive-startup.log` para diagnóstico.

## Validação obrigatória no CI

Uma release só pode ser publicada após:

- Windows portable ser iniciado e permanecer aberto no smoke test.
- Linux ser iniciado em ambiente gráfico virtual no smoke test.
- Runtime MSVC, Qt/QML, libobs, plugins de captura e RTMP serem verificados no Windows.
- Dependências QML e `obs-plugins` serem verificadas no pacote Debian.

A validação de hardware específico (GPU/câmera/jogo/servidor RTMP real) continua dependendo de uma máquina física compatível.