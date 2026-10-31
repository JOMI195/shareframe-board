import { Typography } from '@mui/material';
import { visuallyHidden } from '@mui/utils';

// Pages without a visible title still need an h1 for screen readers.
const HiddenPageTitle = ({ children }: { children: string }) => (
    <Typography component="h1" sx={visuallyHidden}>{children}</Typography>
);

export default HiddenPageTitle;
