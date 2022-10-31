import { Component, Input } from '@angular/core';
import { ComponentFixture, TestBed } from '@angular/core/testing';

import { SharedModule } from '~/app/shared/shared.module';
import { configureTestBed } from '~/testing/unit-test-helper';
import { StonefsDetailComponent } from './stonefs-detail.component';

@Component({ selector: 'cd-stonefs-chart', template: '' })
class StonefsChartStubComponent {
  @Input()
  mdsCounter: any;
}

describe('StonefsDetailComponent', () => {
  let component: StonefsDetailComponent;
  let fixture: ComponentFixture<StonefsDetailComponent>;

  const updateDetails = (
    standbys: string,
    pools: any[],
    ranks: any[],
    mdsCounters: object,
    name: string
  ) => {
    component.data = {
      standbys,
      pools,
      ranks,
      mdsCounters,
      name
    };
    fixture.detectChanges();
  };

  configureTestBed({
    imports: [SharedModule],
    declarations: [StonefsDetailComponent, StonefsChartStubComponent]
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StonefsDetailComponent);
    component = fixture.componentInstance;
    updateDetails('b', [], [], { a: { name: 'a', x: [0], y: [0, 1] } }, 'someFs');
    fixture.detectChanges();
    component.ngOnChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('prepares standby on change', () => {
    expect(component.standbys).toEqual([{ key: 'Standby daemons', value: 'b' }]);
  });
});
