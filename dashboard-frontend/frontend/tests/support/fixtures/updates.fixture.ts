import type { UpdateHistoryEntry, UpdateStatus } from '@/store/updates/updates.Slice';

export interface ReleaseFixture {
  version: string;
  release_notes: string;
  release_date: string;
  criticality: string;
}

export const makeRelease = (over: Partial<ReleaseFixture> = {}): ReleaseFixture => ({
  version: '1.3.0',
  release_notes: 'Bugfixes',
  release_date: '2026-09-01T10:00:00Z',
  criticality: 'normal',
  ...over,
});

export const makeUpdateStatus = (over: Partial<UpdateStatus> = {}): UpdateStatus => ({
  phase: 'idle',
  progress: 0,
  error: '',
  current_version: '1.2.0',
  target_version: '',
  booted_slot: 'A',
  committed_slot: 'A',
  pending_slot: '',
  committed: true,
  ...over,
});

export const makeUpdateHistoryEntry = (over: Partial<UpdateHistoryEntry> = {}): UpdateHistoryEntry => ({
  timestamp: '2026-08-01T10:00:00Z',
  from_version: '1.1.0',
  to_version: '1.2.0',
  result: 'committed',
  error: '',
  ...over,
});
