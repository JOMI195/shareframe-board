import { useNavigate } from 'react-router';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import { getGeneralSettingsUrl, getHomeUrl } from '@/assets/endpoints/app/appEndpoints';
import FactoryResetSteps from './factoryResetSteps';

const FactoryResetPage = () => {
    const navigate = useNavigate();

    return (
        <ShareframeInfoCard
            title="Auf Werkseinstellungen zurücksetzen"
            sx={{ width: '100%' }}
            sections={[{
                content: {
                    type: 'reactNode',
                    value: <FactoryResetSteps signedIn actionsAside onLeave={() => navigate(getHomeUrl() + getGeneralSettingsUrl())} />,
                },
            }]}
        />
    );
};

export default FactoryResetPage;
