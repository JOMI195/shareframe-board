import { ToggleButton, ToggleButtonGroup } from '@mui/material';

export type EpaperView = 'original' | 'epaper';

const options: { value: EpaperView; label: string }[] = [
    { value: 'original', label: 'Original' },
    { value: 'epaper', label: 'E-Paper' },
];

interface EpaperToggleProps {
    value: EpaperView;
    onChange: (value: EpaperView) => void;
}

const EpaperToggle: React.FC<EpaperToggleProps> = ({ value, onChange }) => (
    <ToggleButtonGroup
        value={value}
        exclusive
        onChange={(_event, next: EpaperView | null) => {
            if (next) onChange(next);
        }}
        size="small"
        aria-label="Ansicht"
    >
        {options.map((option) => (
            <ToggleButton key={option.value} value={option.value} sx={{ textTransform: 'none' }}>
                {option.label}
            </ToggleButton>
        ))}
    </ToggleButtonGroup>
);

export default EpaperToggle;
