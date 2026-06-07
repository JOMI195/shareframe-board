FROM node:18

# Set working directory
WORKDIR /app/frontend

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copy package.json and package-lock.json
COPY frontend/package*.json ./

# Install npm dependencies
RUN npm install -g npm@latest
RUN npm install

# Copy the rest of the frontend code
COPY frontend/ .

# Expose the port the app runs on
EXPOSE 3000

# Command to run the application in development mode
CMD ["npm", "run", "dev"]