# Cerve

Um servidor HTTP básico escrito em C para fins de estudo, focado em entender sobre sockets, concorrência e o protocolo HTTP.

## Recursos implementados

- Servidor TCP com sockets POSIX (`socket`, `bind`, `listen`, `accept`)
- Concorrência via `fork()`, com reaping de processos zumbis
- Parsing manual de requisições HTTP (método, path, versão)
- Serving de arquivos estáticos do disco, com `Content-Length` dinâmico
- Detecção de MIME type por extensão (`.html`, `.css`, `.js`, `.png`, `.jpg`, com fallback)
- Proteção contra path traversal (`realpath()` + verificação de boundary)
- Resposta 404 para arquivos inexistentes

## Como testar

```bash
gcc cerve.c -o cerve
./cerve
```

Servidor sobe na porta `4040`. Coloque os arquivos a serem servidos (ex: `index.html`) no mesmo diretório do binário.

## Recursos de estudo

- [Making Minimalist Web Server in C on Linux](https://www.youtube.com/watch?v=2HrYIl6GpYg) — Nir Lichtman
- [Beej's Guide to Network Programming - Using Internet Sockets](https://beej.us/guide/bgnet/) — Brian "Beej Jorgensen" Hall