import React, { useEffect } from 'react';
import { Box, FormControlLabel, Grid, Stack, Switch, Typography } from '@mui/material';
import { PersonOutlined } from '@mui/icons-material';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchDisplayCustomization, selectDisplayCustomization, updateShowSender } from '@/store/displayCustomization/displayCustomization.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const Customization: React.FC = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { showSender, isFetching, isUpdating } = useAppSelector(selectDisplayCustomization);

    useEffect(() => {
        dispatch(fetchDisplayCustomization());
    }, [dispatch]);

    return (
        <Grid container spacing={3}>
            <Grid size={12}>
                <ShareframeInfoCard
                    title="Anzeige"
                    minHeight="100px"
                    sections={[
                        {
                            content: {
                                type: 'reactNode',
                                value: (
                                    <Stack spacing={2}>
                                        <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                            <PersonOutlined sx={{ mr: 1 }} />
                                            <Typography variant="body2">
                                                Zeigt oben rechts auf dem Bild an, wer es geschickt hat. Wird beim nächsten Bildwechsel übernommen.
                                            </Typography>
                                        </Box>
                                        <FormControlLabel
                                            control={
                                                <Switch
                                                    checked={showSender}
                                                    onChange={(e) => dispatch(updateShowSender(e.target.checked))}
                                                    disabled={!isConnected || isFetching || isUpdating}
                                                />
                                            }
                                            label="Name des Absenders anzeigen"
                                        />
                                    </Stack>
                                )
                            }
                        }
                    ]}
                />
            </Grid>
        </Grid>
    );
};

export default Customization;
