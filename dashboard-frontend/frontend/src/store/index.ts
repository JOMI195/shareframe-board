import { configureStore } from '@reduxjs/toolkit';
import { ThunkDispatch } from 'redux-thunk';
import { Action } from 'redux';
import { TypedUseSelectorHook, useDispatch, useSelector } from 'react-redux';
import {
  persistStore,
  persistReducer,
  PersistConfig,
  FLUSH,
  REHYDRATE,
  PAUSE,
  PERSIST,
  PURGE,
  REGISTER,
  createMigrate,
  PersistedState,
} from 'redux-persist';
import storage from 'redux-persist/lib/storage';
import rootReducer from './rootReducer';
import autoMergeLevel2 from 'redux-persist/es/stateReconciler/autoMergeLevel2';
import migrations from './migrations';

const persistConfig: PersistConfig<any> = {
  key: 'shareframe-dashboard-data',
  version: 3,
  storage,
  stateReconciler: autoMergeLevel2,
  migrate: createMigrate(migrations, { debug: import.meta.env.VITE_APP_PRODUCTION === "False" ? true : false }),
};

const persistedReducer = persistReducer(persistConfig, rootReducer as any);

export const store = configureStore({
  reducer: persistedReducer,
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware({
      serializableCheck: {
        ignoredActions: [FLUSH, REHYDRATE, PAUSE, PERSIST, PURGE, REGISTER],
      },
    }),
});

export const persistor = persistStore(store);

export type RootState = ReturnType<typeof rootReducer> & PersistedState;

export type AppDispatch = typeof store.dispatch & ThunkDispatch<RootState, undefined, Action>;;

export const useAppDispatch = () => useDispatch<AppDispatch>();
export const useAppSelector: TypedUseSelectorHook<RootState> = useSelector;
