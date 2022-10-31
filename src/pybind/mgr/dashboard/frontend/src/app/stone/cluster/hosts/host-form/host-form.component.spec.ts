import { HttpClientTestingModule } from '@angular/common/http/testing';
import { ComponentFixture, fakeAsync, TestBed, tick } from '@angular/core/testing';
import { ReactiveFormsModule } from '@angular/forms';
import { RouterTestingModule } from '@angular/router/testing';

import { NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { ToastrModule } from 'ngx-toastr';

import { LoadingPanelComponent } from '~/app/shared/components/loading-panel/loading-panel.component';
import { SharedModule } from '~/app/shared/shared.module';
import { configureTestBed, FormHelper } from '~/testing/unit-test-helper';
import { HostFormComponent } from './host-form.component';

describe('HostFormComponent', () => {
  let component: HostFormComponent;
  let fixture: ComponentFixture<HostFormComponent>;
  let formHelper: FormHelper;

  configureTestBed(
    {
      imports: [
        SharedModule,
        HttpClientTestingModule,
        RouterTestingModule,
        ReactiveFormsModule,
        ToastrModule.forRoot()
      ],
      declarations: [HostFormComponent],
      providers: [NgbActiveModal]
    },
    [LoadingPanelComponent]
  );

  beforeEach(() => {
    fixture = TestBed.createComponent(HostFormComponent);
    component = fixture.componentInstance;
    component.ngOnInit();
    formHelper = new FormHelper(component.hostForm);
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('should open the form in a modal', () => {
    const nativeEl = fixture.debugElement.nativeElement;
    expect(nativeEl.querySelector('cd-modal')).not.toBe(null);
  });

  it('should validate the network address is valid', fakeAsync(() => {
    formHelper.setValue('addr', '115.42.150.37', true);
    tick();
    formHelper.expectValid('addr');
  }));

  it('should show error if network address is invalid', fakeAsync(() => {
    formHelper.setValue('addr', '666.10.10.20', true);
    tick();
    formHelper.expectError('addr', 'pattern');
  }));

  it('should submit the network address', () => {
    component.hostForm.get('addr').setValue('127.0.0.1');
    fixture.detectChanges();
    component.submit();
    expect(component.addr).toBe('127.0.0.1');
  });

  it('should validate the labels are added', () => {
    const labels = ['label1', 'label2'];
    component.hostForm.get('labels').patchValue(labels);
    fixture.detectChanges();
    component.submit();
    expect(component.allLabels).toBe(labels);
  });

  it('should select maintenance mode', () => {
    component.hostForm.get('maintenance').setValue(true);
    fixture.detectChanges();
    component.submit();
    expect(component.status).toBe('maintenance');
  });

  it('should expand the hostname correctly', () => {
    component.hostForm.get('hostname').setValue('stone-node-00.stonelab.com');
    fixture.detectChanges();
    component.submit();
    expect(component.hostnameArray).toStrictEqual(['stone-node-00.stonelab.com']);

    component.hostnameArray = [];

    component.hostForm.get('hostname').setValue('stone-node-[00-10].stonelab.com');
    fixture.detectChanges();
    component.submit();
    expect(component.hostnameArray).toStrictEqual([
      'stone-node-00.stonelab.com',
      'stone-node-01.stonelab.com',
      'stone-node-02.stonelab.com',
      'stone-node-03.stonelab.com',
      'stone-node-04.stonelab.com',
      'stone-node-05.stonelab.com',
      'stone-node-06.stonelab.com',
      'stone-node-07.stonelab.com',
      'stone-node-08.stonelab.com',
      'stone-node-09.stonelab.com',
      'stone-node-10.stonelab.com'
    ]);

    component.hostnameArray = [];

    component.hostForm.get('hostname').setValue('stone-node-00.stonelab.com,stone-node-1.stonelab.com');
    fixture.detectChanges();
    component.submit();
    expect(component.hostnameArray).toStrictEqual([
      'stone-node-00.stonelab.com',
      'stone-node-1.stonelab.com'
    ]);

    component.hostnameArray = [];

    component.hostForm
      .get('hostname')
      .setValue('stone-mon-[01-05].lab.com,stone-osd-[1-4].lab.com,stone-rgw-[001-006].lab.com');
    fixture.detectChanges();
    component.submit();
    expect(component.hostnameArray).toStrictEqual([
      'stone-mon-01.lab.com',
      'stone-mon-02.lab.com',
      'stone-mon-03.lab.com',
      'stone-mon-04.lab.com',
      'stone-mon-05.lab.com',
      'stone-osd-1.lab.com',
      'stone-osd-2.lab.com',
      'stone-osd-3.lab.com',
      'stone-osd-4.lab.com',
      'stone-rgw-001.lab.com',
      'stone-rgw-002.lab.com',
      'stone-rgw-003.lab.com',
      'stone-rgw-004.lab.com',
      'stone-rgw-005.lab.com',
      'stone-rgw-006.lab.com'
    ]);

    component.hostnameArray = [];

    component.hostForm
      .get('hostname')
      .setValue('stone-(mon-[00-04],osd-[001-005],rgw-[1-3]).lab.com');
    fixture.detectChanges();
    component.submit();
    expect(component.hostnameArray).toStrictEqual([
      'stone-mon-00.lab.com',
      'stone-mon-01.lab.com',
      'stone-mon-02.lab.com',
      'stone-mon-03.lab.com',
      'stone-mon-04.lab.com',
      'stone-osd-001.lab.com',
      'stone-osd-002.lab.com',
      'stone-osd-003.lab.com',
      'stone-osd-004.lab.com',
      'stone-osd-005.lab.com',
      'stone-rgw-1.lab.com',
      'stone-rgw-2.lab.com',
      'stone-rgw-3.lab.com'
    ]);
  });
});
