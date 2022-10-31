import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';

import { SharedModule } from '../shared/shared.module';
import { StonefsModule } from './stonefs/stonefs.module';
import { ClusterModule } from './cluster/cluster.module';
import { DashboardModule } from './dashboard/dashboard.module';
import { NfsModule } from './nfs/nfs.module';
import { PerformanceCounterModule } from './performance-counter/performance-counter.module';

@NgModule({
  imports: [
    CommonModule,
    ClusterModule,
    DashboardModule,
    PerformanceCounterModule,
    StonefsModule,
    NfsModule,
    SharedModule
  ],
  declarations: []
})
export class StoneModule {}
