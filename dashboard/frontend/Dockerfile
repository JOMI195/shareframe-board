FROM node:24.2.0-alpine

WORKDIR /app/frontend

COPY frontend/package*.json ./

RUN npm install -g npm@latest
RUN npm install

COPY frontend/ .

EXPOSE 3000

CMD ["npm", "run", "dev"]