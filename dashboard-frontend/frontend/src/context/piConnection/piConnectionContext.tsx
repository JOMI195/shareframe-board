import { fetchWithTimeout } from '@/common/utils/fetch';
import { getSystemHealthUrl } from '@/assets/endpoints/api/frame';
import React, { useState, useEffect, useCallback } from 'react';
import { PiConnectionContext } from './piConnectionContextValue';
import { pollWhileVisible } from '@/common/utils/poll';

export const PiConnectionProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [isConnected, setIsConnected] = useState<boolean>(false);

    // The dashboard *is* the board, so "reachable" == the server answered.
    const checkPiConnection = useCallback(() =>
        fetchWithTimeout(getSystemHealthUrl())
            .then((response) => response.json())
            .then((payload) => setIsConnected(!!payload?.data?.running))
            .catch((error) => {
                console.error('Error checking Pi connection:', error);
                setIsConnected(false);
            }), []);

    // Periodically check connection
    useEffect(() => {
        return pollWhileVisible(checkPiConnection, 30000);
    }, [checkPiConnection]);

    return (
        <PiConnectionContext.Provider
            value={{
                isConnected,
                checkPiConnection
            }}
        >
            {children}
        </PiConnectionContext.Provider>
    );
};
