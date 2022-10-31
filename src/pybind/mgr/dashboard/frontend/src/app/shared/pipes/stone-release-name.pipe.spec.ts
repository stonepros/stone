import { StoneReleaseNamePipe } from './stone-release-name.pipe';

describe('StoneReleaseNamePipe', () => {
  const pipe = new StoneReleaseNamePipe();

  it('create an instance', () => {
    expect(pipe).toBeTruthy();
  });

  it('recognizes a stable release', () => {
    const value =
      'stone version 13.2.1 \
       (5533ecdc0fda920179d7ad84e0aa65a127b20d77) mimic (stable)';
    expect(pipe.transform(value)).toBe('mimic');
  });

  it('recognizes a development release as the master branch', () => {
    const value =
      'stone version 13.1.0-534-g23d3751b89 \
       (23d3751b897b31d2bda57aeaf01acb5ff3c4a9cd) nautilus (dev)';
    expect(pipe.transform(value)).toBe('master');
  });

  it('transforms with wrong version format', () => {
    const value = 'foo';
    expect(pipe.transform(value)).toBe('foo');
  });
});
