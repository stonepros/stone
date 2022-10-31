export abstract class ApiClient {
  getVersionHeaderValue(major: number, minor: number) {
    return `application/vnd.stone.api.v${major}.${minor}+json`;
  }
}
