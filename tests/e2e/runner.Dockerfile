FROM mcr.microsoft.com/playwright:v1.55.1-noble
WORKDIR /e2e
COPY package*.json ./
RUN npm ci
