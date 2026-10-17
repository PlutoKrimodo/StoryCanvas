export type CredentialCapability = 'llm' | 'image'

export interface CredentialModelOption {
  id: string
  label: string
}

export interface CredentialProvider {
  id: string
  label: string
  description: string
  api_base: string
  key_url: string
  custom_base: boolean
  models: CredentialModelOption[]
}

export interface CredentialCapabilityCatalog {
  label: string
  description: string
  providers: CredentialProvider[]
}

export interface CredentialCatalog {
  llm: CredentialCapabilityCatalog
  image: CredentialCapabilityCatalog
}

export interface UserAiCredential {
  capability: CredentialCapability
  capability_label: string
  provider: string
  provider_label: string
  api_key_hint: string
  api_base: string | null
  model: string | null
  updated_at: string
}

export interface AiCredentialOverview {
  byok_enabled: boolean
  encryption_ready: boolean
  quota_exempt: boolean
  credentials: {
    llm: UserAiCredential | null
    image: UserAiCredential | null
  }
}

export interface SaveCredentialPayload {
  provider: string
  /** 留空表示沿用已保存的 Key */
  api_key?: string
  api_base?: string
  model?: string
}

export interface CredentialTestRequest extends SaveCredentialPayload {
  capability: CredentialCapability
}

export interface CredentialTestResult {
  ok: boolean
  verified: boolean
  latency_ms: number
  message: string
}
