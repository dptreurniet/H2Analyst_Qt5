# H2Analyst
 Data visualization and analysis tool, built for Forze Hydrogen Racing.


## Code Structure

The code for this application is structured in the following way:

- **Core**
  - **H2Analyst**  
    Root of the application
  - **Namespace**  
    The namespace contains all constant definitions that are used throughout the application.
  - **DataStore**  
    Back-end of data management within the application. Users interact with the DataStore through the DataPanel.
  - **DataStructures**  
    Definition of the data structures used throughout the application to store the loaded data.
  - **SettingsManager**  
    The settings manager takes care of reading and writing to external settings files.
- **Widgets**
  - **ControlPanel**  
    The ControlPanel is the widget that contains buttons and other control elements to interact with the application.
  - **DataPanel**  
    The DataPanel is the front-end that allows users to interact with loaded data.
    - **TreeView**  
        The TreeView is the main method of presenting the data in the DataStore to the user. It generates a folder structure based on the name of the datasets with systems and subsystems. 
  - **PlotManager**  
    This widget takes care of the creation and placement of plots in the application.
    - **Plot**
      - **Crosshairs**  
        PlotCrosshairs replace the cursor when hovering a plot. They show a horizontal and vertical line, intersecting the cursor, with labels displaying the position on the axis. This is handy when quickly scanning data and checking the values of data at specific times by hand.
      - **Rubberband**  
        The Rubberband is a rectangle that can be dragged to define an area in a plot to zoom towards. It can also snap to horizontal or vertical zooming when the aspect ratio of the rectangle is small or when holding down a key.
      - **TimeCursor**  
        The TimeCursor can be placed at a specific timestamp and indicates the values of all datasets shown at this moment. Its position is synchronized between all active plots to allow the user to check all data values at the same moment. 
  - **Dialogs**
    - **Standards**  
        Standard dialogs are used to do simple things like display a message or ask a yes/no question.
    - **PlotLayout**  
        The PlotLayout dialog is used to set the desired layout of plots.
- **Utilities**
  - **Parsers**  
    Parsers are used to load data of various types by converting them to the data structures used by H2Analyst.
    - **IntCanLog**  
      IntCanLog files are generated by the Forze 8.
  - **DataPopulator**  
    The DataPopulator is a utility that allows fast loading of IntCanLog data by offloading the sorting of messages per dataset to a separate thread. It also features a priority list that can be used to prioritze specific datasets on the fly, which allows the user to already start plotting before all data is populated.
  - **DataOperations**  
    DataOperations contain standard functions like resampling.
  - **TimeStamp**  
    TimeStamp contains a handy implementation of a timestamp object that can be used to deal with time.
  - **Exporters**  
    Exporters are used to generate files in different formats and export them to the system.

