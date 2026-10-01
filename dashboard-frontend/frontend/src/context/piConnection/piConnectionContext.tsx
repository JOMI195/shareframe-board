import { fetchWithTimeout } from '@/common/utils/fetch';
import React, { useState, useEffect, useCallback } from 'react';
import { PiConnectionContext } from './piConnectionContextValue';

export const PiConnectionProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [isConnected, setIsConnected] = useState<boolean>(false);

    // The dashboard *is* the board, so "reachable" == the server answered.
    const checkPiConnection = useCallback(() =>
        fetchWithTimeout('/api/system/health')
            .then((response) => response.json())
            .then((payload) => setIsConnected(!!payload?.data?.running))
            .catch((error) => {
                console.error('Error checking Pi connection:', error);
                setIsConnected(false);
            }), []);

    // Periodically check connection
    useEffect(() => {
        checkPiConnection();

        const intervalId = setInterval(checkPiConnection, 30000);

        return () => clearInterval(intervalId);
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
