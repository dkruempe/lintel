# C++ Base Library API
## Version: 1.0.0

### Security
**bearerAuth**  

| apiKey | *API Key* |
| ------ | --------- |
| Name | Authorization |
| In | header |
| Description | Bearer token authentication |

**Schemes:** http

---
### /hello

#### GET
##### Summary

Greets the user

##### Description

Returns a personalized greeting if a valid Bearer token is provided, otherwise returns a generic "Hello World!".

##### Responses

| Code | Description | Schema |
| ---- | ----------- | ------ |
| 200 | A successful response | string |

##### Security

| Security Schema | Scopes |
| --------------- | ------ |
| bearerAuth |  |

### /history/{processName}/{serviceName}/{label}

#### GET
##### Summary

Get history

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| serviceName | path |  | Yes | string |
| label | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /messageQueue/{processName}/{messageQueueName}

#### GET
##### Summary

Get message queue

##### Description

Returns message queues matching the given name patterns as a JSON array.
With the optional query parameters `after` and `limit` the result is
keyset-paginated (ordered by queue name ascending) and returned as an
envelope object `{items, has_more, next_after}`. Follow `next_after` to
retrieve the next page.

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| messageQueueName | path |  | Yes | string |
| after | query | Exclusive lower bound on queue name; use next_after of the previous page. | No | string |
| limit | query | Maximum number of entries per page (default 100, max 1000). | No | integer |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response (array of message queues, or paging envelope when after/limit is given) |
| 400 | Bad request (invalid limit) |
| 401 | Unauthorized |
| 403 | Forbidden |

### /process/processes/{groupName}

#### GET
##### Summary

Get all processes

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| groupName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /process/groups/{groupName}

#### GET
##### Summary

Get all process groups

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| groupName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /process/start

#### POST
##### Summary

Start a process

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /process/stop/{processId}

#### DELETE
##### Summary

Stop a process

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processId | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |
| 501 | Not Implemented |

### /process/terminate/{processId}

#### DELETE
##### Summary

Terminate a process

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processId | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |
| 501 | Not Implemented |

### /shm/segments/{segmentName}

#### GET
##### Summary

Get all segments

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| segmentName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /shm/segments/shrink/{segmentName}

#### PUT
##### Summary

Shrink a segment

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| segmentName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /shm/segments/grow/{segmentName}/{size}

#### PUT
##### Summary

Grow a segment

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| segmentName | path |  | Yes | string |
| size | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /shm/repositories/{repositoryName}/{segmentName}

#### GET
##### Summary

Get all repositories

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| repositoryName | path |  | Yes | string |
| segmentName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /shm/repository/{uuid}

#### GET
##### Summary

Export a repository

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| uuid | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/login

#### POST
##### Summary

User login

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 403 | Forbidden |

### /user/logout

#### DELETE
##### Summary

User logout

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |

### /user/state

#### GET
##### Summary

Get user login state

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |

### /user/groups

#### GET
##### Summary

Get all groups

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/groups/{groupNameOrIsVirtual}

#### GET
##### Summary

Get all groups of group name or is virtual group

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| groupNameOrIsVirtual | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/groups/{groupName}/{isVirtual}

#### GET
##### Summary

Get all groups of group name and is virtual group

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| groupName | path |  | Yes | string |
| isVirtual | path |  | Yes | boolean |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/users

#### GET
##### Summary

Get all users

##### Description

Returns all users as a JSON array. With the optional query parameters
`after` and `limit` the result is keyset-paginated (ordered by user_name
ascending) and returned as an envelope object `{items, has_more, next_after}`.
Follow `next_after` to retrieve the next page.

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| after | query | Exclusive lower bound on user_name; use next_after of the previous page. | No | string |
| limit | query | Maximum number of users per page (default 100, max 1000). | No | integer |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response (array of users, or paging envelope when after/limit is given) |
| 400 | Bad request (invalid limit) |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/users/{userName}

#### GET
##### Summary

Get all users of user name

##### Description

Filters users by a regular expression on user_name. Supports the same optional
keyset-paging parameters (`after`, `limit`) as /user/users; when paging is
active the response is the envelope object `{items, has_more, next_after}`.

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| userName | path |  | Yes | string |
| after | query | Exclusive lower bound on user_name; use next_after of the previous page. | No | string |
| limit | query | Maximum number of users per page (default 100, max 1000). | No | integer |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response (array of users, or paging envelope when after/limit is given) |
| 400 | Bad request (invalid limit) |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/add

#### POST
##### Summary

Add a user

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |
| 406 | Not Acceptable |

### /user/update

#### PUT
##### Summary

Update a user

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/delete

#### DELETE
##### Summary

Delete a user

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/password

#### PUT
##### Summary

Change the password of a user

##### Description

Self-service requires the current (old) password, an admin may omit it. Passwords are transmitted Base64-encoded.

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| body | body |  | Yes | [UserPasswordChangeDto](#userpasswordchangedto) |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/sessions

#### GET
##### Summary

Get all active sessions of the current user

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |

### /user/sessions/{sessionId}

#### DELETE
##### Summary

Revoke an active session

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| sessionId | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |

### /properties/{processName}/{className}/{instanceName}

#### GET
##### Summary

Get all properties

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| className | path |  | Yes | string |
| instanceName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /properties/{processName}/{className}/{instanceName}/{propertyName}

#### GET
##### Summary

Get a property

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| className | path |  | Yes | string |
| instanceName | path |  | Yes | string |
| propertyName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |
| 405 | Method Not Allowed |

#### PUT
##### Summary

Update a property

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| className | path |  | Yes | string |
| instanceName | path |  | Yes | string |
| propertyName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |
| 405 | Method Not Allowed |

## Models

### UserPasswordChangeDto

| Name | Type | Description | Required |
| ---- | ---- | ----------- | -------- |
| user_name | string |  | Yes |
| old_password | string | Current password, Base64-encoded | No |
| new_password | string | New password, Base64-encoded | Yes |

### UserSessionDto

| Name | Type | Description | Required |
| ---- | ---- | ----------- | -------- |
| id | string | Session id | No |
| ip_address | string |  | No |
| user_name | string |  | No |
| last_access | string |  | No |
