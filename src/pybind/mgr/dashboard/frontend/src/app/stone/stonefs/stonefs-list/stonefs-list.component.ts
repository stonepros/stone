import { Component, OnInit } from '@angular/core';

import { StonefsService } from '~/app/shared/api/stonefs.service';
import { ListWithDetails } from '~/app/shared/classes/list-with-details.class';
import { CellTemplate } from '~/app/shared/enum/cell-template.enum';
import { CdTableColumn } from '~/app/shared/models/cd-table-column';
import { CdTableFetchDataContext } from '~/app/shared/models/cd-table-fetch-data-context';
import { CdTableSelection } from '~/app/shared/models/cd-table-selection';
import { CdDatePipe } from '~/app/shared/pipes/cd-date.pipe';

@Component({
  selector: 'cd-stonefs-list',
  templateUrl: './stonefs-list.component.html',
  styleUrls: ['./stonefs-list.component.scss']
})
export class StonefsListComponent extends ListWithDetails implements OnInit {
  columns: CdTableColumn[];
  filesystems: any = [];
  selection = new CdTableSelection();

  constructor(private stonefsService: StonefsService, private cdDatePipe: CdDatePipe) {
    super();
  }

  ngOnInit() {
    this.columns = [
      {
        name: $localize`Name`,
        prop: 'mdsmap.fs_name',
        flexGrow: 2
      },
      {
        name: $localize`Created`,
        prop: 'mdsmap.created',
        flexGrow: 2,
        pipe: this.cdDatePipe
      },
      {
        name: $localize`Enabled`,
        prop: 'mdsmap.enabled',
        flexGrow: 1,
        cellTransformation: CellTemplate.checkIcon
      }
    ];
  }

  loadFilesystems(context: CdTableFetchDataContext) {
    this.stonefsService.list().subscribe(
      (resp: any[]) => {
        this.filesystems = resp;
      },
      () => {
        context.error();
      }
    );
  }

  updateSelection(selection: CdTableSelection) {
    this.selection = selection;
  }
}
