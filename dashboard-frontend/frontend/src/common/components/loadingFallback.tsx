import { Box, CircularProgress } from '@mui/material';

const LoadingFallback = () => (
    <Box sx={{ minHeight: '100dvh', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <CircularProgress aria-label="Seite lädt" />
    </Box>
);

export default LoadingFallback;
