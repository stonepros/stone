import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';

import { TreeModule } from '@circlon/angular-tree-component';
import { NgbNavModule } from '@ng-bootstrap/ng-bootstrap';
import { ChartsModule } from 'ng2-charts';

import { AppRoutingModule } from '~/app/app-routing.module';
import { SharedModule } from '~/app/shared/shared.module';
import { StonefsChartComponent } from './stonefs-chart/stonefs-chart.component';
import { StonefsClientsComponent } from './stonefs-clients/stonefs-clients.component';
import { StonefsDetailComponent } from './stonefs-detail/stonefs-detail.component';
import { StonefsDirectoriesComponent } from './stonefs-directories/stonefs-directories.component';
import { StonefsListComponent } from './stonefs-list/stonefs-list.component';
import { StonefsTabsComponent } from './stonefs-tabs/stonefs-tabs.component';

@NgModule({
  imports: [CommonModule, SharedModule, AppRoutingModule, ChartsModule, TreeModule, NgbNavModule],
  declarations: [
    StonefsDetailComponent,
    StonefsClientsComponent,
    StonefsChartComponent,
    StonefsListComponent,
    StonefsTabsComponent,
    StonefsDirectoriesComponent
  ]
})
export class StonefsModule {}
