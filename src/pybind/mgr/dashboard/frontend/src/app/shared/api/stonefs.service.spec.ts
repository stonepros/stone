import { HttpClientTestingModule, HttpTestingController } from '@angular/common/http/testing';
import { TestBed } from '@angular/core/testing';

import { configureTestBed } from '~/testing/unit-test-helper';
import { StonefsService } from './stonefs.service';

describe('StonefsService', () => {
  let service: StonefsService;
  let httpTesting: HttpTestingController;

  configureTestBed({
    imports: [HttpClientTestingModule],
    providers: [StonefsService]
  });

  beforeEach(() => {
    service = TestBed.inject(StonefsService);
    httpTesting = TestBed.inject(HttpTestingController);
  });

  afterEach(() => {
    httpTesting.verify();
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });

  it('should call list', () => {
    service.list().subscribe();
    const req = httpTesting.expectOne('api/stonefs');
    expect(req.request.method).toBe('GET');
  });

  it('should call getStonefs', () => {
    service.getStonefs(1).subscribe();
    const req = httpTesting.expectOne('api/stonefs/1');
    expect(req.request.method).toBe('GET');
  });

  it('should call getClients', () => {
    service.getClients(1).subscribe();
    const req = httpTesting.expectOne('api/stonefs/1/clients');
    expect(req.request.method).toBe('GET');
  });

  it('should call getTabs', () => {
    service.getTabs(2).subscribe();
    const req = httpTesting.expectOne('ui-api/stonefs/2/tabs');
    expect(req.request.method).toBe('GET');
  });

  it('should call getMdsCounters', () => {
    service.getMdsCounters('1').subscribe();
    const req = httpTesting.expectOne('api/stonefs/1/mds_counters');
    expect(req.request.method).toBe('GET');
  });

  it('should call lsDir', () => {
    service.lsDir(1).subscribe();
    const req = httpTesting.expectOne('ui-api/stonefs/1/ls_dir?depth=2');
    expect(req.request.method).toBe('GET');
    service.lsDir(2, '/some/path').subscribe();
    httpTesting.expectOne('ui-api/stonefs/2/ls_dir?depth=2&path=%252Fsome%252Fpath');
  });

  it('should call mkSnapshot', () => {
    service.mkSnapshot(3, '/some/path').subscribe();
    const req = httpTesting.expectOne('api/stonefs/3/snapshot?path=%252Fsome%252Fpath');
    expect(req.request.method).toBe('POST');

    service.mkSnapshot(4, '/some/other/path', 'snap').subscribe();
    httpTesting.expectOne('api/stonefs/4/snapshot?path=%252Fsome%252Fother%252Fpath&name=snap');
  });

  it('should call rmSnapshot', () => {
    service.rmSnapshot(1, '/some/path', 'snap').subscribe();
    const req = httpTesting.expectOne('api/stonefs/1/snapshot?path=%252Fsome%252Fpath&name=snap');
    expect(req.request.method).toBe('DELETE');
  });

  it('should call updateQuota', () => {
    service.quota(1, '/some/path', { max_bytes: 1024 }).subscribe();
    let req = httpTesting.expectOne('api/stonefs/1/quota?path=%252Fsome%252Fpath');
    expect(req.request.method).toBe('PUT');
    expect(req.request.body).toEqual({ max_bytes: 1024 });

    service.quota(1, '/some/path', { max_files: 10 }).subscribe();
    req = httpTesting.expectOne('api/stonefs/1/quota?path=%252Fsome%252Fpath');
    expect(req.request.method).toBe('PUT');
    expect(req.request.body).toEqual({ max_files: 10 });

    service.quota(1, '/some/path', { max_bytes: 1024, max_files: 10 }).subscribe();
    req = httpTesting.expectOne('api/stonefs/1/quota?path=%252Fsome%252Fpath');
    expect(req.request.method).toBe('PUT');
    expect(req.request.body).toEqual({ max_bytes: 1024, max_files: 10 });
  });
});
