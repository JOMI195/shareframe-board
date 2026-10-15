import React from 'react';
import { ToggleButton, ToggleButtonGroup, useMediaQuery, useTheme } from '@mui/material';
import { ShuffleOutlined, ScheduleOutlined, NewReleasesOutlined, DragIndicatorOutlined } from '@mui/icons-material';
import { PlaybackMode } from '@/types';

interface PlaybackOrderSelectProps {
    value: PlaybackMode | null;
    disabled?: boolean;
    onChange: (mode: PlaybackMode) => void;
}

const OPTIONS: { mode: PlaybackMode; label: string; icon: React.ReactNode }[] = [
    { mode: 'shuffle', label: 'Zufällig', icon: <ShuffleOutlined fontSize="small" /> },
    { mode: 'chronological', label: 'Chronologisch', icon: <ScheduleOutlined fontSize="small" /> },
    { mode: 'newest_first', label: 'Neueste zuerst', icon: <NewReleasesOutlined fontSize="small" /> },
    { mode: 'custom', label: 'Eigene Reihenfolge', icon: <DragIndicatorOutlined fontSize="small" /> },
];

const PlaybackOrderSelect: React.FC<PlaybackOrderSelectProps> = ({ value, disabled = false, onChange }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <ToggleButtonGroup
            value={value}
            exclusive
            fullWidth
            disabled={disabled}
            orientation={isSmallScreen ? 'vertical' : 'horizontal'}
            onChange={(_, mode: PlaybackMode | null) => { if (mode) onChange(mode); }}
            aria-label="Wiedergabereihenfolge"
            size="small"
        >
            {OPTIONS.map(({ mode, label, icon }) => (
                <ToggleButton key={mode} value={mode} sx={{ gap: 1, textTransform: 'none' }}>
                    {icon}
                    {label}
                </ToggleButton>
            ))}
        </ToggleButtonGroup>
    );
};

export default PlaybackOrderSelect;
