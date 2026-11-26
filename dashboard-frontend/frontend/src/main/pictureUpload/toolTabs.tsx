import React from 'react';
import { Tab, Tabs } from '@mui/material';
import CropIcon from '@mui/icons-material/Crop';
import TuneIcon from '@mui/icons-material/Tune';

export type Tool = 'crop' | 'adjust';

const tools: { value: Tool; label: string; icon: React.ReactElement }[] = [
    { value: 'crop', label: 'Zuschnitt', icon: <CropIcon /> },
    { value: 'adjust', label: 'Anpassen', icon: <TuneIcon /> },
];

interface ToolTabsProps {
    value: Tool;
    onChange: (tool: Tool) => void;
    disabled: boolean;
}

const ToolTabs: React.FC<ToolTabsProps> = ({ value, onChange, disabled }) => (
    <Tabs
        value={value}
        onChange={(_event, tool: Tool) => onChange(tool)}
        variant="fullWidth"
        aria-label="Werkzeuge"
        sx={{
            gridArea: 'tabs',
            position: { xs: 'sticky', md: 'static' },
            top: 0,
            zIndex: 1,
            bgcolor: 'background.paper',
            borderBottom: 1,
            borderColor: 'divider',
        }}
    >
        {tools.map(({ value: tool, label, icon }) => (
            <Tab
                key={tool}
                value={tool}
                label={label}
                icon={icon}
                iconPosition="start"
                disabled={disabled}
                sx={{ minHeight: 48, textTransform: 'none' }}
            />
        ))}
    </Tabs>
);

export default ToolTabs;
