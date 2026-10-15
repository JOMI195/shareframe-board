import React from 'react';
import { Box, Button, Typography } from '@mui/material';
import { PhotoLibraryOutlined } from '@mui/icons-material';
import { Link } from 'react-router';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import { getImageFileUrl } from '@/assets/endpoints/api/frame';
import { getSlideshowPageUrl } from '@/assets/endpoints/app/appEndpoints';
import { INowShowing } from '@/types';

const describe = (shown: INowShowing | null): string => {
    if (!shown || shown.state === 'empty') return 'Noch nichts angezeigt';
    if (shown.state === 'cleared') return 'Display geleert';
    if (shown.imageId !== null) return shown.sender ? `Bild von ${shown.sender}` : 'Bild aus der Diashow';
    return 'Standardbild oder Systemanzeige';
};

const NowShowingCard: React.FC<{ nowShowing: INowShowing | null }> = ({ nowShowing }) => {
    const imageId = nowShowing?.state === 'image' ? nowShowing.imageId : null;

    return (
        <ShareframeInfoCard
            title="Aktuell angezeigt"
            minHeight="100px"
            sections={[
                {
                    content: {
                        type: 'reactNode',
                        value: (
                            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
                                {imageId !== null && (
                                    <Box sx={{ display: 'flex', justifyContent: 'center', lineHeight: 0 }}>
                                        <AuthenticatedImage
                                            url={getImageFileUrl(imageId)}
                                            alt="Aktuell angezeigtes Bild"
                                            style={{ maxWidth: '100%', maxHeight: 360, borderRadius: 8, display: 'block' }}
                                        />
                                    </Box>
                                )}
                                <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 1, flexWrap: 'wrap' }}>
                                    <Typography variant="body2" color="text.secondary">{describe(nowShowing)}</Typography>
                                    <Button
                                        component={Link}
                                        to={`/${getSlideshowPageUrl()}`}
                                        size="small"
                                        startIcon={<PhotoLibraryOutlined />}
                                    >
                                        Zur Diashow
                                    </Button>
                                </Box>
                            </Box>
                        )
                    }
                }
            ]}
        />
    );
};

export default NowShowingCard;
