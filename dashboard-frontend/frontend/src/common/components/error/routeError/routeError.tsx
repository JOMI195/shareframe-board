import { useEffect } from 'react';
import { Button, Stack, Typography } from '@mui/material';
import { Link, useRouteError } from 'react-router';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';
import { isStaleChunkError, reloadForStaleChunk } from '@/common/utils/staleChunkReload';
import Logo from '../../logo';
import StandaloneTheme from '../standaloneTheme';

const RouteError = () => {
    const error = useRouteError();
    const staleChunk = isStaleChunkError(error);

    useEffect(() => {
        if (staleChunk) reloadForStaleChunk();
        else if (import.meta.env.VITE_APP_PRODUCTION === 'False') console.error(error);
    }, [error, staleChunk]);

    return (
        <StandaloneTheme>
            <Stack
                spacing={2}
                sx={{ minHeight: '70dvh', py: 4, px: 2, justifyContent: 'center', alignItems: 'center', textAlign: 'center' }}
            >
                <Logo
                    darkLogoSrc="/frame-3d-no-data.svg"
                    lightLogoSrc="/frame-3d-no-data.svg"
                    clickable={false}
                    marginRight={0}
                    maxWidth={180}
                    alt=""
                />
                <Typography variant="h5" component="h1">
                    {staleChunk ? 'Es gibt eine neue Version' : 'Etwas ist schiefgelaufen'}
                </Typography>
                <Typography sx={{ color: 'text.secondary', maxWidth: 420 }}>
                    {staleChunk
                        ? 'Lade die Seite neu, um die Einstellungen zu aktualisieren.'
                        : 'Diese Seite konnte nicht angezeigt werden. Lade sie neu oder geh zurück zur Übersicht.'}
                </Typography>
                <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1.5} sx={{ pt: 1 }}>
                    <Button variant="contained" onClick={() => window.location.reload()}>
                        Neu laden
                    </Button>
                    <Button variant="outlined" component={Link} to={getHomeUrl()} reloadDocument>
                        Zur Übersicht
                    </Button>
                </Stack>
            </Stack>
        </StandaloneTheme>
    );
};

export default RouteError;
