# Proyecto MiniHTTPd

### **Estudiante:** Lindsay Valentina Guzmán Quilumba ###

Servidor HTTP/1.1 escrito en C para Linux. Atiende solicitudes GET de archivos estáticos con sockets no bloqueantes y epoll.

## Compilación y ejecución

Desde la raíz del proyecto, con GNU Make instalado, ejecute:

```sh
make
./minihttpd
```
Abra `http://localhost:8000/` en el navegador o haga pruebas desde la terminal:

```sh
curl -i http://localhost:8000/
curl -i http://localhost:8000/style.css
curl -i -X POST http://localhost:8000/
```

Para limpiar ejecute `make clean`

## Procesamiento de solicitudes HTTP/1.1

- Acumula bytes por conexión con read hasta llegar al delimitador del encabezado.
- Permite un limite de `4096` bytes para línea de solicitud y encabezados y rutas de hasta `256` bytes.
- Atiende solicitudes GET y responde `405 Method Not Allowed` con otros metodos.
- Exige el encabezado `Host` una sola vez, acepta `User-Agent`, usa keep-alive por defecto y devuelve 400 si GET tiene Content-Length distinto de cero.
- Valida las rutas con realpath() dentro de www/ y bloquea accesos fuera de la raíz publica.

## Tipos de MIME

| Extensión | Content-Type |
| --- | --- |
| `.html, .htm` | text/html |
| `.css` | text/css |
| `.js` | application/javascript |
| `.png` | image/png |
| `.jpg, .jpeg` | image/jpeg|
| otros | application/octet-stream|