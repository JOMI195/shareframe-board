import {defineConfig} from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';
import compression from 'vite-plugin-compression2';

const envDir = path.resolve(__dirname, '../');
const buildDir = path.resolve(__dirname, '../build/');

export default defineConfig(function (_a) {
    const mode = _a.mode;
    return {
        envDir: envDir,
        plugins: [
            react(),
            compression({
                algorithm: 'brotliCompress',
                include: /\.(js|css|html|svg|json|txt|ico|xml)$/,
                deleteOriginalAssets: false,
            }),
        ],
        server: {
            host: true,
            port: 3000,
            watch: {
                usePolling: true,
            },
        },
        build: {
            outDir: buildDir,
            emptyOutDir: true,
            sourcemap: false
        },
        resolve: {
            alias: {
                '@': path.resolve(__dirname, 'src'),
            },
        },
        optimizeDeps: {
            include: ['@mui/material/Tooltip', '@emotion/styled', '@emotion/react'],
        },
        define: {
            'process.env': {
                MODE: mode,
            },
        },
    };
});
