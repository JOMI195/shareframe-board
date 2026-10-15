FROM node:24-alpine AS build
WORKDIR /app
COPY .env.prod ./
COPY frontend/package*.json frontend/
RUN cd frontend && npm ci
COPY frontend/ frontend/
RUN cd frontend && npm run build

FROM nginx:1.27-alpine
COPY --from=build /app/build /usr/share/nginx/html
COPY --from=e2e nginx.conf /etc/nginx/conf.d/default.conf
HEALTHCHECK --interval=1s --timeout=2s --retries=30 CMD wget -qO- http://127.0.0.1/ >/dev/null
