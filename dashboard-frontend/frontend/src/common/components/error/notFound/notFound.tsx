import { Button, Stack, Typography, useMediaQuery, useTheme } from "@mui/material";
import { Link } from "react-router";
import Logo from "../../logo";

const NotFound = () => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <Stack
            spacing={2}
            sx={{
                minHeight: '70dvh',
                width: '100%',
                px: 2,
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center',
                textAlign: 'center',
            }}
        >
            <Logo
                darkLogoSrc="/frame-3d-no-data.svg"
                lightLogoSrc="/frame-3d-no-data.svg"
                clickable={false}
                marginRight={0}
                maxWidth={isSmallScreen ? 150 : 300}
                alt=""
            />
            <Typography variant="h2" component="h1">404</Typography>
            <Typography variant="h6" component="p" sx={{
                color: "text.secondary"
            }}>
                Seite nicht gefunden
            </Typography>
            <Button
                component={Link}
                to="/"
                variant="contained"
                sx={{ mt: 1 }}
            >
                Zur Startseite
            </Button>
        </Stack>
    );
};

export default NotFound;
