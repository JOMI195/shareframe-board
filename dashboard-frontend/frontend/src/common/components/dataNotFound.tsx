import { Stack, Typography, useMediaQuery, useTheme } from "@mui/material";
import Logo from "./logo";

interface DataNotFoundProps {
    notFoundMessage: string;
}
const DataNotFound: React.FC<DataNotFoundProps> = ({ notFoundMessage }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

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
            <Logo
                darkLogoSrc="/frame-3d-no-data.svg"
                lightLogoSrc="/frame-3d-no-data.svg"
                clickable={false}
                maxWidth={isSmallScreen ? 150 : 300}
            />
            <Typography variant="h6">{notFoundMessage}</Typography>
        </Stack>
    );
}

export default DataNotFound;