import { ReactNode } from "react";
import { Box, Button, Stack, Typography, useMediaQuery, useTheme } from "@mui/material";
import Logo from "./logo";

interface DataNotFoundProps {
    notFoundMessage: string;
    // The next step out of the empty state, e.g. trying again.
    action?: { label: string; onClick: () => void; icon?: ReactNode };
}
const DataNotFound: React.FC<DataNotFoundProps> = ({ notFoundMessage, action }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const imageWidth = isSmallScreen ? 150 : 300;

    return (
        <Stack
            spacing={1}
            sx={{
                width: "100%",
                display: "flex",
                justifyContent: "center",
                alignItems: "center",
                textAlign: "center"
            }}
        >
            <Box sx={{ position: "relative", display: "inline-flex" }}>
                <Logo
                    darkLogoSrc="/frame-3d-no-data.svg"
                    lightLogoSrc="/frame-3d-no-data.svg"
                    clickable={false}
                    maxWidth={imageWidth}
                    alt=""
                />
            </Box>
            <Typography variant="h6" component="p">{notFoundMessage}</Typography>
            {action && (
                <Button variant="contained" startIcon={action.icon} onClick={action.onClick} sx={{ mt: 1 }}>
                    {action.label}
                </Button>
            )}
        </Stack>
    );
}

export default DataNotFound;