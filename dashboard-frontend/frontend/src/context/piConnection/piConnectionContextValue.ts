import { createContext, useContext } from 'react';

interface PiConnectionContextType {
    isConnected: boolean;
    checked: boolean;
    checkPiConnection: () => Promise<void>;
}

export const PiConnectionContext = createContext<PiConnectionContextType>({
    isConnected: false,
    checked: false,
    checkPiConnection: async () => { },
});

export const usePiConnection = () => {
    const context = useContext(PiConnectionContext);

    if (!context) {
        throw new Error('usePiConnection must be used within a PiConnectionProvider');
    }

    return context;
};
