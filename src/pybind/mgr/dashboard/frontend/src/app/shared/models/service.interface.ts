export interface StoneServiceStatus {
  container_image_id: string;
  container_image_name: string;
  size: number;
  running: number;
  last_refresh: Date;
  created: Date;
}

// This will become handy when creating arbitrary services
export interface StoneServiceSpec {
  service_name: string;
  service_type: string;
  service_id: string;
  unmanaged: boolean;
  status: StoneServiceStatus;
  spec: StoneServiceAdditionalSpec;
  placement: StoneServicePlacement;
}

export interface StoneServiceAdditionalSpec {
  backend_service: string;
  api_user: string;
  api_password: string;
  api_port: number;
  api_secure: boolean;
  rgw_frontend_port: number;
  trusted_ip_list: string[];
  virtual_ip: string;
  frontend_port: number;
  monitor_port: number;
  virtual_interface_networks: string[];
  pool: string;
  rgw_frontend_ssl_certificate: string;
  ssl: boolean;
  ssl_cert: string;
  ssl_key: string;
}

export interface StoneServicePlacement {
  count: number;
  placement: string;
  hosts: string[];
  label: string;
}
