import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component, Input } from '@angular/core';
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';

import { CdTableSelection } from '~/app/shared/models/cd-table-selection';
import { SharedModule } from '~/app/shared/shared.module';
import { configureTestBed } from '~/testing/unit-test-helper';
import { StonefsListComponent } from './stonefs-list.component';

@Component({ selector: 'cd-stonefs-tabs', template: '' })
class StonefsTabsStubComponent {
  @Input()
  selection: CdTableSelection;
}

describe('StonefsListComponent', () => {
  let component: StonefsListComponent;
  let fixture: ComponentFixture<StonefsListComponent>;

  configureTestBed({
    imports: [BrowserAnimationsModule, SharedModule, HttpClientTestingModule],
    declarations: [StonefsListComponent, StonefsTabsStubComponent]
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StonefsListComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
