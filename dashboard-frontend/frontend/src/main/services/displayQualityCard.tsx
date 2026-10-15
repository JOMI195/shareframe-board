import React, { useEffect } from 'react';
import { FormControlLabel, Switch, Typography } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchDisplayQuality, selectDisplayQualityState, updateDisplayQuality } from '@/store/displayQuality/displayQuality.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const DisplayQualityCard: React.FC = () => {
    const dispatch = useAppDispatch();
    const { quality, updating } = useAppSelector(selectDisplayQualityState);
    const { isConnected } = usePiConnection();

    useEffect(() => {
        dispatch(fetchDisplayQuality());
    }, [dispatch]);

    return (
        <ShareframeInfoCard
            title="Bildqualität"
            minHeight="auto"
            sections={[{
                content: {
                    type: 'reactNode',
                    value: (
                        <>
                            <FormControlLabel
                                label="4 Graustufen (Test)"
                                control={
                                    <Switch
                                        checked={quality === 'gray4'}
                                        disabled={quality === null || updating || !isConnected}
                                        onChange={(e) => dispatch(updateDisplayQuality(e.target.checked ? 'gray4' : 'mono'))}
                                    />
                                }
                            />
                            <Typography variant="body2" sx={{ color: 'text.secondary' }}>
                                Gilt ab dem nächsten Bildwechsel. Nur Panels ab 24.10.2023.
                            </Typography>
                        </>
                    ),
                },
            }]}
        />
    );
};

export default DisplayQualityCard;
