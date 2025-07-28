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

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| processName | path |  | Yes | string |
| messageQueueName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
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

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
| 401 | Unauthorized |
| 403 | Forbidden |

### /user/users/{userName}

#### GET
##### Summary

Get all users of user name

##### Parameters

| Name | Located in | Description | Required | Schema |
| ---- | ---------- | ----------- | -------- | ------ |
| userName | path |  | Yes | string |

##### Responses

| Code | Description |
| ---- | ----------- |
| 200 | A successful response |
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
