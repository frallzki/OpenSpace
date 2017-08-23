import React, { Component } from 'react';
import PropTypes from 'prop-types';
import Pane from './Pane';
import LoadingString from '../common/LoadingString/LoadingString';
import FilterList from '../common/FilterList/FilterList';
import DataManager from '../../api/DataManager';
import { AllPropertiesKey } from '../../api/keys';
import PropertyCollection from './Properties/PropertyCollection';

import styles from './SettingsPane.scss';

class SettingsPane extends Component {
  constructor(props) {
    super(props);
    this.state = { properties: [], hasData: false };

    this.receiveData = this.receiveData.bind(this);
  }

  componentDidMount() {
    DataManager.getValue(AllPropertiesKey, this.receiveData);
  }

  receiveData({ value }) {
    this.setState({ properties: value, hasData: true });
  }

  render() {
    const properties = this.state.properties
      .map(prop => Object.assign(prop, { key: prop.name }));

    return (
      <Pane title="Settings" closeCallback={this.props.closeCallback}>
        { !this.state.hasData && (
          <LoadingString loading>
            Loading...
          </LoadingString>
        )}

        { properties.length > 0 && (
          <FilterList
            data={properties}
            className={styles.list}
            viewComponent={PropertyCollection}
          />
        )}
      </Pane>
    );
  }
}

SettingsPane.propTypes = {
  closeCallback: PropTypes.func,
};

SettingsPane.defaultProps = {
  closeCallback: null,
};

export default SettingsPane;
