import React from 'react';
import {
    Card,
    CardContent,
    Typography,
    Box,
    Divider,
    CardProps,
    CardActions,
} from '@mui/material';

interface Section {
    label?: string;
    content:
    | string
    | number
    | null
    | undefined
    | {
        type: 'reactNode';
        value: React.ReactNode;
    };
}

interface ShareframeInfoCardProps extends Omit<CardProps, 'elevation'> {
    title?: string;
    sections?: Section[];
    elevation?: number;
    minHeight?: string;
    actions?: React.ReactNode; // ✅ Add support for actions
}

const ShareframeInfoCard: React.FC<ShareframeInfoCardProps> = ({
    title,
    sections = [],
    elevation = 1,
    minHeight = "150px",
    actions,
    ...cardProps
}) => {
    return (
        <Card elevation={elevation} sx={{ height: '100%', display: 'flex', flexDirection: 'column', ...cardProps.sx }}>
            <CardContent sx={{
                minHeight,
                flex: 1,
                display: 'flex',
                flexDirection: 'column'
            }}>
                {title && (
                    <Typography variant="h6" color="text.secondary" gutterBottom>
                        {title}
                    </Typography>
                )}

                <Box display="flex" flexDirection="column" justifyContent="space-between" sx={{ flex: 1 }}>
                    {sections.map((section, index) => {
                        const { label, content } = section;

                        let contentNode: React.ReactNode = null;

                        if (typeof content === 'string' || typeof content === 'number') {
                            contentNode = (
                                <Typography variant="body2" gutterBottom>
                                    {content}
                                </Typography>
                            );
                        } else if (content && typeof content === 'object' && content.type === 'reactNode') {
                            contentNode = content.value;
                        }

                        if (!contentNode) return null;

                        return (
                            <Box key={index} sx={{ mb: index < sections.length - 1 ? 2 : 0 }}>
                                {label && (
                                    <>
                                        <Typography variant="body2" sx={{ mb: 1 }}>
                                            {label}
                                        </Typography>
                                        <Divider sx={{ mb: 1 }} />
                                    </>
                                )}
                                <Box>{contentNode}</Box>
                            </Box>
                        );
                    })}
                </Box>
            </CardContent>

            {actions && (
                <CardActions>
                    {actions}
                </CardActions>
            )}
        </Card>
    );
};

export default ShareframeInfoCard;
