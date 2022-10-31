import { InventoryDevice } from '~/app/stone/cluster/inventory/inventory-devices/inventory-device.model';

export interface DevicesSelectionClearEvent {
  type: string;
  clearedDevices: InventoryDevice[];
}
