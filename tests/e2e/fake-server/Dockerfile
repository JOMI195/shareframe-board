FROM node:24-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --omit=dev
COPY server.mjs ./
HEALTHCHECK --interval=1s --timeout=2s --retries=30 CMD wget -qO- http://127.0.0.1:9000/__test/state >/dev/null
CMD ["node", "server.mjs"]
