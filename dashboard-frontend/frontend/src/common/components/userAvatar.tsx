import { Avatar, SxProps, Theme, useTheme } from '@mui/material';
import { stringToColor } from '@/common/utils/color/color';

interface UserAvatarProps {
    username: string;
    size?: number;
    sx?: SxProps<Theme>;
}

const UserAvatar = ({ username, size = 40, sx }: UserAvatarProps) => {
    const theme = useTheme();
    const background = stringToColor(username);

    return (
        // Decorative: the username is always shown next to it.
        <Avatar
            alt={username}
            aria-hidden
            sx={[
                {
                    width: size,
                    height: size,
                    fontSize: size * 0.45,
                    bgcolor: background,
                    color: theme.palette.getContrastText(background),
                },
                ...(Array.isArray(sx) ? sx : [sx]),
            ]}
        >
            {username.slice(0, 1).toUpperCase()}
        </Avatar>
    );
};

export default UserAvatar;
