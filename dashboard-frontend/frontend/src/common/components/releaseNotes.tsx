import { List, ListItem, Typography } from '@mui/material';
import { parseReleaseNotes } from '@/common/utils/releaseNotes';

const ReleaseNotes = ({ notes }: { notes: string }) => (
    <>
        {parseReleaseNotes(notes).map((block, index) =>
            block.type === 'list' ? (
                <List key={index} dense disablePadding sx={{ listStyleType: 'disc', pl: 2.5 }}>
                    {block.items.map((item, itemIndex) => (
                        <ListItem key={itemIndex} disablePadding sx={{ display: 'list-item' }}>
                            <Typography variant="body2">{item}</Typography>
                        </ListItem>
                    ))}
                </List>
            ) : (
                <Typography key={index} variant="body2" gutterBottom>
                    {block.text}
                </Typography>
            )
        )}
    </>
);

export default ReleaseNotes;
