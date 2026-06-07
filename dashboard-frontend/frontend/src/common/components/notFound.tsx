import { Box, Typography } from "@mui/material";

const NotFound = () => {
    return (
        <Box
            sx={{
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center',
                minHeight: '100vh',
            }}
        >
            <Typography variant="h5" style={{ borderRight: '0.01em solid black', padding: '0.5em', margin: '0.5em' }}>
                404
            </Typography>
            <Typography variant="h5">
                Page not found
            </Typography>
        </Box>
    );
};

export default NotFound;