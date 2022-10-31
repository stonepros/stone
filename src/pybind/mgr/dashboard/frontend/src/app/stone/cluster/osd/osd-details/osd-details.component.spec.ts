import { HttpClientTestingModule } from '@angular/common/http/testing';
import { ComponentFixture, TestBed } from '@angular/core/testing';

import { NgbNavModule } from '@ng-bootstrap/ng-bootstrap';

import { TablePerformanceCounterComponent } from '~/app/stone/performance-counter/table-performance-counter/table-performance-counter.component';
import { StoneSharedModule } from '~/app/stone/shared/stone-shared.module';
import { SharedModule } from '~/app/shared/shared.module';
import { configureTestBed } from '~/testing/unit-test-helper';
import { OsdDetailsComponent } from './osd-details.component';

describe('OsdDetailsComponent', () => {
  let component: OsdDetailsComponent;
  let fixture: ComponentFixture<OsdDetailsComponent>;

  configureTestBed({
    imports: [HttpClientTestingModule, NgbNavModule, SharedModule, StoneSharedModule],
    declarations: [OsdDetailsComponent, TablePerformanceCounterComponent]
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(OsdDetailsComponent);
    component = fixture.componentInstance;
    component.selection = undefined;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
