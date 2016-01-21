const async = require('async')
const bacnet = require('../bacnet.js')
const r = bacnet.init({
  datalink: {
    iface: 'bridge100',
    ip_port: 0xBAC0
  },
  device: true
})

function objectIdToString (objectId) {
  return bacnet.objectTypeToString(objectId.type) + '/' + objectId.instance
}

const propertyKeys = ['object-identifier', 'object-name', 'description', 'system-status', 'vendor-name',
  'vendor-identifier', 'model-name', 'firmware-revision', 'application-software-version', 'location', 'local-time', 'local-date',
  'utc-offset', 'daylight-savings-status', 'protocol-version', 'protocol-revision',
  'protocol-services-supported'
]

function receiveObjectList (err, property) {
  if (err) return console.log('ERROR', err)
  console.log('Received property /', objectIdToString(property.object), '/', bacnet.propertyKeyToString(property.property))
  async.mapSeries(property.value, function (objectId, objectRead) {
    async.mapSeries(propertyKeys, (propertyName, propertyRead) =>
        r.readProperty('127.0.0.1', objectId.type, objectId.instance, propertyName, false, (err, propertyValue) => {
          if (err) {
            propertyRead(null, 'NONE')
          } else {
            propertyRead(null, propertyValue.value)
          }
        }),
      (err, values) => {
        if (err) {
          console.log('error', err)
          return objectRead(null, false)
        }
        objectRead(null, propertyKeys.reduce((result, key, index) => {
          if (values[index].length = 1) result[key] = values[index][0]
          else result[key] = values[index]
          return result
        }, {}))
      })
  }, (err, result) => {
    if (err) {
      console.log('Error', err)
    }
    console.log('Result', result)
  })
}

r.on('error', err => console.log('error in bacnet', err))

r.readProperty('127.0.0.1', 'device', 260001, 'object-list', false, receiveObjectList)

setTimeout(function () {}, 1000)
