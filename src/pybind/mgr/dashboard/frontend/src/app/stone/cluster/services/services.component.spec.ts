import { HttpClientTestingModule } from '@angular/common/http/testing';
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';

import { ToastrModule } from 'ngx-toastr';
import { of } from 'rxjs';

import { StoneModule } from '~/app/stone/stone.module';
import { CoreModule } from '~/app/core/core.module';
import { StoneServiceService } from '~/app/shared/api/stone-service.service';
import { OrchestratorService } from '~/app/shared/api/orchestrator.service';
import { CdTableFetchDataContext } from '~/app/shared/models/cd-table-fetch-data-context';
import { Permissions } from '~/app/shared/models/permissions';
import { AuthStorageService } from '~/app/shared/services/auth-storage.service';
import { SharedModule } from '~/app/shared/shared.module';
import { configureTestBed } from '~/testing/unit-test-helper';
import { ServicesComponent } from './services.component';

describe('ServicesComponent', () => {
  let component: ServicesComponent;
  let fixture: ComponentFixture<ServicesComponent>;

  const fakeAuthStorageService = {
    getPermissions: () => {
      return new Permissions({ hosts: ['read'] });
    }
  };

  const services = [
    {
      service_type: 'osd',
      service_name: 'osd',
      status: {
        size: 3,
        running: 3,
        last_refresh: '2020-02-25T04:33:26.465699'
      }
    },
    {
      service_type: 'crash',
      service_name: 'crash',
      status: {
        size: 1,
        running: 1,
        last_refresh: '2020-02-25T04:33:26.465766'
      }
    }
  ];

  configureTestBed({
    imports: [
      BrowserAnimationsModule,
      StoneModule,
      CoreModule,
      SharedModule,
      HttpClientTestingModule,
      RouterTestingModule,
      ToastrModule.forRoot()
    ],
    providers: [{ provide: AuthStorageService, useValue: fakeAuthStorageService }]
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(ServicesComponent);
    component = fixture.componentInstance;
    const orchService = TestBed.inject(OrchestratorService);
    const stoneServiceService = TestBed.inject(StoneServiceService);
    spyOn(orchService, 'status').and.returnValue(of({ available: true }));
    spyOn(stoneServiceService, 'list').and.returnValue(of(services));
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('should have columns that are sortable', () => {
    expect(
      component.columns
        // Filter the 'Expand/Collapse Row' column.
        .filter((column) => !(column.cellClass === 'cd-datatable-expand-collapse'))
        // Filter the 'Placement' column.
        .filter((column) => !(column.prop === ''))
        .every((column) => Boolean(column.prop))
    ).toBeTruthy();
  });

  it('should return all services', () => {
    component.getServices(new CdTableFetchDataContext(() => undefined));
    expect(component.services.length).toBe(2);
  });

  it('should not display doc panel if orchestrator is available', () => {
    expect(component.showDocPanel).toBeFalsy();
  });
});
