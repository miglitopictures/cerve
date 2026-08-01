# Cerve

Um servidor HTTP básico escrito em C para fins de estudo, focado em entender sobre sockets, concorrência e o protocolo HTTP.

## Motivação

No último ano, desenvolvi alguns sites de portfólio (SPA) para colegas designers e animadores, buscando um domínio mais sólido das tecnologias web antes de adentrar no mundo dos frameworks.

Para hospedar esses sites, sempre utilizo o GitHub Pages, pela gratuidade (para projetos pequenos) e pela facilidade no deploy. Aprendi, eventualmente, que o servidor do GH redireciona automaticamente para uma `404.html` quando uma rota não existe. Simular esse comportamento localmente no VS Code com o [Live Server](https://ritwickdey.github.io/vscode-live-server/) sempre foi um problema e deu muita dor de cabeça na hora de administrar as rotas dinâmicas.

Para o site que estou desenvolvendo agora, usei o Sonnet 5 para programar um servidor com esse comportamento, e ele resolveu o problema na hora, com pouquíssimas linhas de Python... Fiquei com inveja e com vontade de entender como servidores HTTP funcionavam. O Cerve é basicamente uma recriação manual desse script Python, escrito em C, com algumas funcionalidades que servem para o meu workflow pessoal!

## Recursos implementados

- Servidor TCP com sockets POSIX (`socket`, `bind`, `listen`, `accept`)
- Concorrência via `fork()`, com reaping de processos zumbis
- Parsing manual de requisições HTTP (método, path, versão)
- Serving de arquivos estáticos do disco, com `Content-Length` dinâmico
- Detecção de MIME type por extensão (`.html`, `.css`, `.js`, `.png`, `.jpg`, com fallback)
- Proteção contra path traversal (`realpath()` no arquivo requisitado + verificação de boundary contra o diretório raiz)
- Resposta 404 para arquivos inexistentes, com página de fallback customizável
- Configuração via flags de linha de comando (`getopt`)

## Como testar

```bash
gcc cerve.c -o cerve
./cerve
```

Servidor sobe na porta `4040` por padrão. Coloque os arquivos a serem servidos (ex: `index.html`) no mesmo diretório do binário.

## Flags de configuração

| Flag | Argumento | Descrição |
|------|-----------|-----------|
| `-p` | obrigatório | Porta em que o servidor vai escutar (ex: `-p 8080`) |
| `-f` | opcional | Ativa redirecionamento em 404 para uma página de fallback. Sem argumento, usa `404.html` como padrão. Com argumento, usa o arquivo especificado (deve estar colado à flag, ex: `-fcustom404.html`, sem espaço) |

Exemplos:

```bash
./cerve -p 8080               # sobe na porta 8080, 404 padrão (texto puro)
./cerve -f                    # 404 redireciona para "404.html"
./cerve -p 8080 -fcustom.html # porta 8080, 404 redireciona para "custom.html"
```

## Limitações conhecidas
 
Esse projeto tem fins de estudo e uso pessoal (substituindo o live server para projetos spa hosteados no Github Pages), decisões priorizaram simplicidade/aprendizado em vez de robustez de produção. Dito isso, listo algumas limitações que percebo:
 
- **Concorrência via `fork()` por conexão**: para uso de desenvolvimento (localhost), não tem muito problema o fato do `fork()` duplicar os recursos do processo, mas acredito que isso se torna impraticável num servidor real com multiplas conexões.
- **Interpretação de request HTTP simplificado**: só lê os primeiros `BUFFER_SIZE` (256) bytes da requisição em uma única chamada de `read()`, sem tratar requests maiores ou fragmentados em múltiplos pacotes TCP. (ainda não fiz um site que precisou de mais do que isso, mas é preciso validar com mais calma e rever se necessário)
- **Suporte a métodos HTTP**: o servidor não distingue métodos (`GET`, `POST`, etc.) — qualquer request é tratado como se fosse `GET`, servindo o arquivo do path indicado.
- **Tabela de MIME type reduzida**: cobre apenas as extensões mais comuns para os sites que sirvo (`.html`, `.css`, `.js`, `.png`, `.jpg`), com fallback genérico para o resto. É quase trivial colocar mais opções, vou populando com o tempo.

## Recursos de estudo

- [Making Minimalist Web Server in C on Linux](https://www.youtube.com/watch?v=2HrYIl6GpYg) - Nir Lichtman
- [Beej's Guide to Network Programming - Using Internet Sockets](https://beej.us/guide/bgnet/) - Brian "Beej Jorgensen" Hall
- [getopt() man page](https://man7.org/linux/man-pages/man3/getopt.3.html) - Linux man-pages 6.18
- [getopt() function in C to parse command line arguments](https://www.geeksforgeeks.org/c/getopt-function-in-c-to-parse-command-line-arguments/) - GeeksforGeeks