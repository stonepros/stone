import { PageHelper } from '../page-helper.po';

export class FilesystemsPageHelper extends PageHelper {
  pages = { index: { url: '#/stonefs', id: 'cd-stonefs-list' } };
}
