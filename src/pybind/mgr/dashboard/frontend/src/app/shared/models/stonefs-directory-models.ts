import { TreeStatus } from '@swimlane/ngx-datatable';

export class StonefsSnapshot {
  name: string;
  path: string;
  created: string;
}

export class StonefsQuotas {
  max_bytes?: number;
  max_files?: number;
}

export class StonefsDir {
  name: string;
  path: string;
  quotas: StonefsQuotas;
  snapshots: StonefsSnapshot[];
  parent: string;
  treeStatus?: TreeStatus; // Needed for table tree view
}
