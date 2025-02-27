/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

const basePath          = '../../../../';
const testPath          = 'src/app/tests/suites/';
const certificationPath = 'src/app/tests/suites/certification/';
const zapPath           = basePath + 'third_party/zap/repo/';
const YAML              = require(zapPath + 'node_modules/yaml');
const fs                = require('fs');
const path              = require('path');

// Import helpers from zap core
const templateUtil = require(zapPath + 'dist/src-electron/generator/template-util.js')

const { DelayCommands }                 = require('./simulated-clusters/TestDelayCommands.js');
const { LogCommands }                   = require('./simulated-clusters/TestLogCommands.js');
const { Clusters, asBlocks, asPromise } = require('./ClustersHelper.js');
const { asUpperCamelCase }              = require(basePath + 'src/app/zap-templates/templates/app/helper.js');

const kClusterName       = 'cluster';
const kEndpointName      = 'endpoint';
const kCommandName       = 'command';
const kWaitCommandName   = 'wait';
const kIndexName         = 'index';
const kValuesName        = 'values';
const kConstraintsName   = 'constraints';
const kArgumentsName     = 'arguments';
const kResponseName      = 'response';
const kDisabledName      = 'disabled';
const kResponseErrorName = 'error';

class NullObject {
  toString()
  {
    return "YOU SHOULD HAVE CHECKED (isLiteralNull definedValue)"
  }
};

function throwError(test, errorStr)
{
  console.error('Error in: ' + test.filename + '.yaml for test with label: "' + test.label + '"\n');
  console.error(errorStr);
  throw new Error();
}

function setDefault(test, name, defaultValue)
{
  if (!(name in test)) {
    if (defaultValue == null) {
      const errorStr = 'Test does not have any "' + name + '" defined.';
      throwError(test, errorStr);
    }

    test[name] = defaultValue;
  }
}

function setDefaultType(test)
{
  if (kWaitCommandName in test) {
    setDefaultTypeForWaitCommand(test);
  } else {
    setDefaultTypeForCommand(test);
  }
}

function setDefaultTypeForWaitCommand(test)
{
  const type = test[kWaitCommandName];
  switch (type) {
  case 'readAttribute':
    test.isAttribute     = true;
    test.isReadAttribute = true;
    break;
  case 'writeAttribute':
    test.isAttribute      = true;
    test.isWriteAttribute = true;
    break;
  case 'subscribeAttribute':
    test.isAttribute          = true;
    test.isSubscribeAttribute = true;
    break;
  default:
    test.isCommand = true;
    break;
  }

  test.isWait = true;
}

function setDefaultTypeForCommand(test)
{
  const type = test[kCommandName];
  switch (type) {
  case 'readAttribute':
    test.commandName     = 'Read';
    test.isAttribute     = true;
    test.isReadAttribute = true;
    break;

  case 'writeAttribute':
    test.commandName      = 'Write';
    test.isAttribute      = true;
    test.isWriteAttribute = true;
    break;

  case 'subscribeAttribute':
    test.commandName          = 'Subscribe';
    test.isAttribute          = true;
    test.isSubscribeAttribute = true;
    break;

  case 'waitForReport':
    test.commandName     = 'Report';
    test.isAttribute     = true;
    test.isWaitForReport = true;
    break;

  default:
    test.commandName = test.command;
    test.isCommand   = true;
    break;
  }

  test.isWait = false;
}

function setDefaultArguments(test)
{
  const defaultArguments = {};
  setDefault(test, kArgumentsName, defaultArguments);

  const defaultArgumentsValues = [];
  setDefault(test[kArgumentsName], kValuesName, defaultArgumentsValues);

  if (!test.isWriteAttribute) {
    return;
  }

  if (!('value' in test[kArgumentsName])) {
    const errorStr = 'Test does not have a "value" defined.';
    throwError(test, errorStr);
  }

  test[kArgumentsName].values.push({ name : test.attribute, value : test[kArgumentsName].value });
  delete test[kArgumentsName].value;
}

function setDefaultResponse(test)
{
  const defaultResponse = {};
  setDefault(test, kResponseName, defaultResponse);

  const defaultResponseError = 0;
  setDefault(test[kResponseName], kResponseErrorName, defaultResponseError);

  const defaultResponseValues = [];
  setDefault(test[kResponseName], kValuesName, defaultResponseValues);

  const defaultResponseConstraints = {};
  setDefault(test[kResponseName], kConstraintsName, defaultResponseConstraints);

  const hasResponseValue              = 'value' in test[kResponseName];
  const hasResponseConstraints        = 'constraints' in test[kResponseName] && Object.keys(test[kResponseName].constraints).length;
  const hasResponseValueOrConstraints = hasResponseValue || hasResponseConstraints;

  if (test.isCommand && hasResponseValueOrConstraints) {
    const errorStr = 'Test has a "value" or a "constraints" defined.\n' +
        '\n' +
        'Command should explicitly use the response argument name. Example: \n' +
        '- label: "Send Test Specific Command"\n' +
        '  command: "testSpecific"\n' +
        '  response: \n' +
        '    values: \n' +
        '      - name: "returnValue"\n' +
        '      - value: 7\n';
    throwError(test, errorStr);
  }

  // Step that waits for a particular event does not requires constraints nor expected values.
  if (test.isWait) {
    return;
  }

  if (!test.isAttribute) {
    return;
  }

  if (test.isWriteAttribute || test.isSubscribeAttribute) {
    if (hasResponseValueOrConstraints) {
      const errorStr = 'Attribute test has a "value" or a "constraints" defined.';
      throwError(test, errorStr);
    }

    return;
  }

  if (!hasResponseValueOrConstraints) {
    console.log(test);
    console.log(test[kResponseName]);
    const errorStr = 'Test does not have a "value" or a "constraints" defined.';
    throwError(test, errorStr);
  }

  if (hasResponseValue) {
    test[kResponseName].values.push({ name : test.attribute, value : test[kResponseName].value });
  }

  if (hasResponseConstraints) {
    test[kResponseName].values.push({ name : test.attribute, constraints : test[kResponseName].constraints });
  }

  delete test[kResponseName].value;
}

function setDefaults(test, defaultConfig)
{
  const defaultClusterName = defaultConfig[kClusterName] || null;
  const defaultEndpointId  = kEndpointName in defaultConfig ? defaultConfig[kEndpointName] : null;
  const defaultDisabled    = false;

  setDefaultType(test);
  setDefault(test, kClusterName, defaultClusterName);
  setDefault(test, kEndpointName, defaultEndpointId);
  setDefault(test, kDisabledName, defaultDisabled);
  setDefaultArguments(test);
  setDefaultResponse(test);
}

function parse(filename)
{
  let filepath;
  const isCertificationTest = filename.startsWith('Test_TC_');
  if (isCertificationTest) {
    filepath = path.resolve(__dirname, basePath + certificationPath + filename + '.yaml');
  } else {
    filepath = path.resolve(__dirname, basePath + testPath + filename + '.yaml');
  }

  const data = fs.readFileSync(filepath, { encoding : 'utf8', flag : 'r' });
  const yaml = YAML.parse(data);

  // "subscribeAttribute" command expects a report to be acked before
  // it got a success response.
  // In order to validate that the report has been received with the proper value
  // a "subscribeAttribute" command can have a response configured into the test step
  // definition. In this case, a new async "waitForReport" test step will be synthesized
  // and added to the list of tests.
  yaml.tests.forEach((test, index) => {
    if (test.command == "subscribeAttribute" && test.response) {
      // Create a new report test where the expected response is the response argument
      // for the "subscribeAttributeTest"
      const reportTest = {
        label : "Report: " + test.label,
        command : "waitForReport",
        attribute : test.attribute,
        response : test.response,
        async : true
      };
      delete test.response;

      // insert the new report test into the tests list
      yaml.tests.splice(index, 0, reportTest);

      // Associate the "subscribeAttribute" test with the synthesized report test
      test.hasWaitForReport = true;
      test.waitForReport    = reportTest;
    }
  });

  const defaultConfig = yaml.config || [];
  yaml.tests.forEach(test => {
    test.filename = filename;
    test.testName = yaml.name;
    setDefaults(test, defaultConfig);
  });

  // Filter disabled tests
  yaml.tests = yaml.tests.filter(test => !test.disabled);
  yaml.tests.forEach((test, index) => {
    setDefault(test, kIndexName, index);
  });

  yaml.filename   = filename;
  yaml.totalTests = yaml.tests.length;

  return yaml;
}

function printErrorAndExit(context, msg)
{
  console.log(context.testName, ': ', context.label);
  console.log(msg);
  process.exit(1);
}

function getClusters()
{
  // Create a new array to merge the configured clusters list and test
  // simulated clusters.
  return Clusters.getClusters().then(clusters => clusters.concat(DelayCommands, LogCommands));
}

function getCommands(clusterName)
{
  switch (clusterName) {
  case DelayCommands.name:
    return Promise.resolve(DelayCommands.commands);
  case LogCommands.name:
    return Promise.resolve(LogCommands.commands);
  default:
    return Clusters.getClientCommands(clusterName);
  }
}

function getAttributes(clusterName)
{
  switch (clusterName) {
  case DelayCommands.name:
    return Promise.resolve(DelayCommands.attributes);
  case LogCommands.name:
    return Promise.resolve(LogCommands.attributes);
  default:
    return Clusters.getServerAttributes(clusterName);
  }
}

function assertCommandOrAttribute(context)
{
  const clusterName = context.cluster;
  return getClusters().then(clusters => {
    if (!clusters.find(cluster => cluster.name == clusterName)) {
      const names = clusters.map(item => item.name);
      printErrorAndExit(context, 'Missing cluster "' + clusterName + '" in: \n\t* ' + names.join('\n\t* '));
    }

    let filterName;
    let items;

    if (context.isCommand) {
      filterName = context.command;
      items      = getCommands(clusterName);
    } else if (context.isAttribute) {
      filterName = context.attribute;
      items      = getAttributes(clusterName);
    } else {
      printErrorAndExit(context, 'Unsupported command type: ', context);
    }

    return items.then(items => {
      const filter = item => item.name.toLowerCase() == filterName.toLowerCase();
      const item          = items.find(filter);
      const itemType      = (context.isCommand ? 'Command' : 'Attribute');

      // If the command or attribute is not found, it could be because of a typo in the test
      // description, or an attribute name not matching the spec, or a wrongly configured zap
      // file.
      if (!item) {
        const names = items.map(item => item.name);
        printErrorAndExit(context, 'Missing ' + itemType + ' "' + filterName + '" in: \n\t* ' + names.join('\n\t* '));
      }

      // If the command or attribute has been found but the response can not be found, it could be
      // because of a wrongly configured cluster definition.
      if (!item.response) {
        printErrorAndExit(context, 'Missing ' + itemType + ' "' + filterName + '" response');
      }

      return item;
    });
  });
}

//
// Templates
//
function chip_tests(list, options)
{
  const items = Array.isArray(list) ? list : list.split(',');
  const names = items.map(name => name.trim());
  const tests = names.map(item => parse(item));
  return templateUtil.collectBlocks(tests, options, this);
}

function chip_tests_items(options)
{
  return templateUtil.collectBlocks(this.tests, options, this);
}

function isTestOnlyCluster(name)
{
  const testOnlyClusters = [
    DelayCommands.name,
    LogCommands.name,
  ];
  return testOnlyClusters.includes(name);
}

function chip_tests_item_response_type(options)
{
  const promise = assertCommandOrAttribute(this).then(item => {
    if (item.hasSpecificResponse) {
      return 'Clusters::' + asUpperCamelCase(this.cluster) + '::Commands::' + asUpperCamelCase(item.response.name)
          + '::DecodableType';
    }

    return 'DataModel::NullObjectType';
  });

  return asPromise.call(this, promise, options);
}

function chip_tests_item_parameters(options)
{
  const commandValues = this.arguments.values;

  const promise = assertCommandOrAttribute(this).then(item => {
    if (this.isAttribute && !this.isWriteAttribute) {
      if (this.isSubscribeAttribute) {
        const minInterval = { name : 'minInterval', type : 'in16u', chipType : 'uint16_t', definedValue : this.minInterval };
        const maxInterval = { name : 'maxInterval', type : 'in16u', chipType : 'uint16_t', definedValue : this.maxInterval };
        return [ minInterval, maxInterval ];
      }
      return [];
    }

    const commandArgs = item.arguments;
    const commands    = commandArgs.map(commandArg => {
      commandArg = JSON.parse(JSON.stringify(commandArg));

      const expected = commandValues.find(value => value.name.toLowerCase() == commandArg.name.toLowerCase());
      if (!expected) {
        if (commandArg.isOptional) {
          return undefined;
        }
        printErrorAndExit(this,
            'Missing "' + commandArg.name + '" in arguments list: \n\t* '
                + commandValues.map(command => command.name).join('\n\t* '));
      }
      // test_cluster_command_value is a recursive partial using #each. At some point the |global|
      // context is lost and it fails. Make sure to attach the global context as a property of the | value |
      // that is evaluated.
      function attachGlobal(global, value)
      {
        if (Array.isArray(value)) {
          value = value.map(v => attachGlobal(global, v));
        } else if (value instanceof Object) {
          for (key in value) {
            if (key == "global") {
              continue;
            }
            value[key] = attachGlobal(global, value[key]);
          }
        } else if (value === null) {
          value = new NullObject();
        } else {
          switch (typeof value) {
          case 'number':
            value = new Number(value);
            break;
          case 'string':
            value = new String(value);
            break;
          case 'boolean':
            value = new Boolean(value);
            break;
          default:
            throw new Error('Unsupported value: ' + JSON.stringify(value));
          }
        }

        value.global = global;
        return value;
      }
      commandArg.definedValue = attachGlobal(this.global, expected.value);

      return commandArg;
    });

    return commands.filter(item => item !== undefined);
  });

  return asBlocks.call(this, promise, options);
}

function chip_tests_item_response_parameters(options)
{
  const responseValues = this.response.values.slice();

  const promise = assertCommandOrAttribute(this).then(item => {
    const responseArgs = item.response.arguments;

    const responses = responseArgs.map(responseArg => {
      responseArg = JSON.parse(JSON.stringify(responseArg));

      const expectedIndex = responseValues.findIndex(value => value.name.toLowerCase() == responseArg.name.toLowerCase());
      if (expectedIndex != -1) {
        const expected = responseValues.splice(expectedIndex, 1)[0];
        if ('value' in expected) {
          responseArg.hasExpectedValue = true;
          responseArg.expectedValue    = expected.value;
        }

        if ('constraints' in expected) {
          responseArg.hasExpectedConstraints = true;
          responseArg.expectedConstraints    = expected.constraints;
        }
      }

      return responseArg;
    });

    const unusedResponseValues = responseValues.filter(response => 'value' in response);
    unusedResponseValues.forEach(unusedResponseValue => {
      printErrorAndExit(this,
          'Missing "' + unusedResponseValue.name + '" in response arguments list:\n\t* '
              + responseArgs.map(response => response.name).join('\n\t* '));
    });

    return responses;
  });

  return asBlocks.call(this, promise, options);
}

function isLiteralNull(value, options)
{
  // Literal null might look different depending on whether it went through
  // attachGlobal or not.
  return (value === null) || (value instanceof NullObject);
}

//
// Module exports
//
exports.chip_tests                          = chip_tests;
exports.chip_tests_items                    = chip_tests_items;
exports.chip_tests_item_parameters          = chip_tests_item_parameters;
exports.chip_tests_item_response_type       = chip_tests_item_response_type;
exports.chip_tests_item_response_parameters = chip_tests_item_response_parameters;
exports.isTestOnlyCluster                   = isTestOnlyCluster;
exports.isLiteralNull                       = isLiteralNull;
