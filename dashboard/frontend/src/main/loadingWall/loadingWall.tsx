import Logo from "@/common/components/logo";
import { useAppSelector } from "@/store";
import { selectLoadingWallState } from "@/store/loadingWall/loadingWall.Slice";
import { Box, Typography } from "@mui/material";

const LoadingWall = () => {
    const { message } = useAppSelector(selectLoadingWallState);

    return (
        <Box
            display="flex"
            flexDirection={"column"}
            justifyContent="center"
            alignItems="center"
            textAlign={"center"}
            flexGrow={1}
            height={"100vh"}
        >
            <Logo
                darkLogoSrc="/logo-dark-full-shareframe.svg"
                lightLogoSrc="/logo-light-full-shareframe.svg"
                maxWidth={200}
                marginRight={0}
                clickable={false}
            />
            <Typography variant="h6" sx={{ p: 2 }}>
                {message}
            </Typography>
        </Box>
    );
}

export default LoadingWall;